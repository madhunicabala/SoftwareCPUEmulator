#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "alu.h"

/* Opcode name table for debug printing */
static const char *opcode_names[] = {
    "NOP","HALT","MOV","LOAD","STORE","PUSH","POP",
    "ADD","SUB","MUL","INC","DEC",
    "AND","OR","XOR","NOT","SHL","SHR",
    "CMP",
    "JMP","JZ","JNZ","JL","JGE","JC",
    "CALL","RET",
    "IN","OUT"
};

static const char *reg_names[] = { "R0", "R1", "R2", "R3" };

/* ============================================================
   Lifecycle
   ============================================================ */
void cpu_init(CPU *cpu, Memory *mem) {
    memset(cpu, 0, sizeof(CPU));
    cpu->mem    = mem;
    cpu->pc     = CODE_SEG_BASE;
    cpu->sp     = STACK_INIT_SP;
    cpu->halted = 0;
}

void cpu_reset(CPU *cpu) {
    memset(cpu->reg, 0, sizeof(cpu->reg));
    cpu->pc          = CODE_SEG_BASE;
    cpu->sp          = STACK_INIT_SP;
    cpu->flags       = 0;
    cpu->halted      = 0;
    cpu->cycle_count = 0;
}

int cpu_load(CPU *cpu, const uint8_t *program, uint32_t size) {
    if (size > CODE_SEG_SIZE) {
        fprintf(stderr, "[CPU] Program too large: %u bytes (max %u)\n",
                size, CODE_SEG_SIZE);
        return -1;
    }
    memcpy(&cpu->mem->data[CODE_SEG_BASE], program, size);
    printf("[CPU] Loaded %u bytes at 0x%04X\n", size, CODE_SEG_BASE);
    return 0;
}

/* ============================================================
   Flag computation
   ============================================================ */
void cpu_set_flags(CPU *cpu, uint16_t result, uint32_t full_result,
                   uint16_t a, uint16_t b, int is_sub) {
    cpu->flags = 0;

    /* Zero flag */
    if (result == 0)
        cpu->flags |= FLAG_Z;

    /* Negative flag — bit 15 set */
    if (result & 0x8000)
        cpu->flags |= FLAG_N;

    /* Carry flag — bit 16 set in full result */
    if (full_result > 0xFFFF)
        cpu->flags |= FLAG_C;

    /* Overflow flag — signed overflow */
    if (is_sub) {
        /* SUB overflow: pos - neg = neg OR neg - pos = pos */
        if (((a ^ b) & 0x8000) && ((a ^ result) & 0x8000))
            cpu->flags |= FLAG_V;
    } else {
        /* ADD overflow: pos + pos = neg OR neg + neg = pos */
        if (!((a ^ b) & 0x8000) && ((a ^ result) & 0x8000))
            cpu->flags |= FLAG_V;
    }
}

/* ============================================================
   Stack operations
   ============================================================ */
void cpu_push(CPU *cpu, uint16_t value) {
    cpu->sp -= 2;
    mem_write_word(cpu->mem, cpu->sp, value);
}

uint16_t cpu_pop(CPU *cpu) {
    uint16_t value = mem_read_word(cpu->mem, cpu->sp);
    cpu->sp += 2;
    return value;
}

/* ============================================================
   FETCH — read 16-bit instruction at PC, advance PC by 2
   ============================================================ */
uint16_t cpu_fetch(CPU *cpu) {
    uint16_t instr = mem_read_word(cpu->mem, cpu->pc);
    cpu->pc += INSTR_SIZE;
    return instr;
}

/* ============================================================
   Helper — resolve operand value based on addressing mode
   ============================================================ */
static uint16_t resolve_src(CPU *cpu, uint16_t instr) {
    uint8_t  mode = INSTR_MODE(instr);
    uint8_t  src  = INSTR_SRC(instr);

    switch (mode) {
        case MODE_REG:
            return cpu->reg[src & 0x03];

        case MODE_IMM:
            /* sign-extend 6-bit immediate */
            return (uint16_t)SIGN_EXT6(src);

        case MODE_DIRECT: {
            /* next word in stream is the full 16-bit address */
            uint16_t addr = mem_read_word(cpu->mem, cpu->pc);
            cpu->pc += INSTR_SIZE;
            return mem_read_word(cpu->mem, addr);
        }

        case MODE_INDIRECT: {
            uint16_t addr = cpu->reg[src & 0x03];
            return mem_read_word(cpu->mem, addr);
        }
    }
    return 0;
}

/* ============================================================
   EXECUTE — decode and run one instruction
   ============================================================ */
void cpu_execute(CPU *cpu, uint16_t instr) {
    uint8_t  opcode = INSTR_OPCODE(instr);
    uint8_t  mode   = INSTR_MODE(instr);
    uint8_t  dst    = INSTR_DST(instr);
    ALUResult ar;

    switch (opcode) {

        /* ── System ────────────────────────────────────── */
        case OP_NOP:
            break;

        case OP_HALT:
            cpu->halted = 1;
            printf("[CPU] HALT at cycle %llu, PC=0x%04X\n",
                   (unsigned long long)cpu->cycle_count, cpu->pc - INSTR_SIZE);
            break;

        /* ── Data Movement ─────────────────────────────── */
        case OP_MOV:
            cpu->reg[dst] = resolve_src(cpu, instr);
            break;

        case OP_LOAD:
            cpu->reg[dst] = resolve_src(cpu, instr);
            break;

        case OP_STORE: {
            uint16_t src_val = cpu->reg[dst]; /* dst field holds source reg */
            if (mode == MODE_DIRECT) {
                uint16_t addr = mem_read_word(cpu->mem, cpu->pc);
                cpu->pc += INSTR_SIZE;
                mem_write_word(cpu->mem, addr, src_val);
            } else if (mode == MODE_INDIRECT) {
                uint16_t addr = cpu->reg[INSTR_SRC(instr) & 0x03];
                mem_write_word(cpu->mem, addr, src_val);
            } else {
                fprintf(stderr, "[CPU] STORE: unsupported mode %u\n", mode);
            }
            break;
        }

        case OP_PUSH:
            cpu_push(cpu, cpu->reg[dst]);
            break;

        case OP_POP:
            cpu->reg[dst] = cpu_pop(cpu);
            break;

        /* ── Arithmetic ────────────────────────────────── */
        case OP_ADD: {
            /* capture operand ONCE — resolve_src advances PC for DIRECT mode */
            uint16_t a = cpu->reg[dst];
            uint16_t operand = resolve_src(cpu, instr);
            ar = alu_add(a, operand);
            cpu_set_flags(cpu, ar.result, ar.full, a, operand, 0);
            cpu->reg[dst] = ar.result;
            break;
        }

        case OP_SUB: {
            uint16_t a = cpu->reg[dst];
            uint16_t operand = resolve_src(cpu, instr);
            ar = alu_sub(a, operand);
            cpu_set_flags(cpu, ar.result, ar.full, a, operand, 1);
            cpu->reg[dst] = ar.result;
            break;
        }

        case OP_MUL: {
            uint16_t a = cpu->reg[dst];
            uint16_t operand = resolve_src(cpu, instr);
            ar = alu_mul(a, operand);
            cpu->reg[dst] = ar.result;
            cpu->flags = 0;
            if (ar.result == 0)       cpu->flags |= FLAG_Z;
            if (ar.result & 0x8000)   cpu->flags |= FLAG_N;
            break;
        }

        case OP_INC: {
            uint16_t a = cpu->reg[dst];
            ar = alu_inc(a);
            cpu_set_flags(cpu, ar.result, ar.full, a, 1, 0);
            cpu->reg[dst] = ar.result;
            break;
        }

        case OP_DEC: {
            uint16_t a = cpu->reg[dst];
            ar = alu_dec(a);
            cpu_set_flags(cpu, ar.result, ar.full, a, 1, 1);
            cpu->reg[dst] = ar.result;
            break;
        }

        /* ── Logic ─────────────────────────────────────── */
        case OP_AND: {
            uint16_t operand = resolve_src(cpu, instr);
            ar = alu_and(cpu->reg[dst], operand);
            cpu->reg[dst] = ar.result;
            cpu->flags = (ar.result == 0) ? FLAG_Z : 0;
            if (ar.result & 0x8000) cpu->flags |= FLAG_N;
            break;
        }

        case OP_OR: {
            uint16_t operand = resolve_src(cpu, instr);
            ar = alu_or(cpu->reg[dst], operand);
            cpu->reg[dst] = ar.result;
            cpu->flags = (ar.result == 0) ? FLAG_Z : 0;
            if (ar.result & 0x8000) cpu->flags |= FLAG_N;
            break;
        }

        case OP_XOR: {
            uint16_t operand = resolve_src(cpu, instr);
            ar = alu_xor(cpu->reg[dst], operand);
            cpu->reg[dst] = ar.result;
            cpu->flags = (ar.result == 0) ? FLAG_Z : 0;
            if (ar.result & 0x8000) cpu->flags |= FLAG_N;
            break;
        }

        case OP_NOT: {
            ar = alu_not(cpu->reg[dst]);
            cpu->reg[dst] = ar.result;
            cpu->flags = (ar.result == 0) ? FLAG_Z : 0;
            if (ar.result & 0x8000) cpu->flags |= FLAG_N;
            break;
        }

        case OP_SHL:
            ar = alu_shl(cpu->reg[dst], (uint8_t)(INSTR_SRC(instr) & 0x0F));
            cpu->reg[dst] = ar.result;
            cpu->flags = 0;
            if (ar.result == 0) cpu->flags |= FLAG_Z;
            if (ar.full > 0xFFFF) cpu->flags |= FLAG_C;
            break;

        case OP_SHR:
            ar = alu_shr(cpu->reg[dst], (uint8_t)(INSTR_SRC(instr) & 0x0F));
            cpu->reg[dst] = ar.result;
            cpu->flags = (ar.result == 0) ? FLAG_Z : 0;
            break;

        /* ── Compare ───────────────────────────────────── */
        case OP_CMP: {
            uint16_t operand = resolve_src(cpu, instr);
            ar = alu_cmp(cpu->reg[dst], operand);
            cpu_set_flags(cpu, ar.result, ar.full, cpu->reg[dst], operand, 1);
            /* result is discarded — only flags matter */
            break;
        }

        /* ── Jumps ─────────────────────────────────────── */
        case OP_JMP: {
            uint16_t target = mem_read_word(cpu->mem, cpu->pc);
            cpu->pc = target;
            break;
        }
        case OP_JZ: {
            uint16_t target = mem_read_word(cpu->mem, cpu->pc);
            cpu->pc = (cpu->flags & FLAG_Z) ? target : cpu->pc + INSTR_SIZE;
            break;
        }
        case OP_JNZ: {
            uint16_t target = mem_read_word(cpu->mem, cpu->pc);
            cpu->pc = !(cpu->flags & FLAG_Z) ? target : cpu->pc + INSTR_SIZE;
            break;
        }
        case OP_JL: {
            uint16_t target = mem_read_word(cpu->mem, cpu->pc);
            cpu->pc = (cpu->flags & FLAG_N) ? target : cpu->pc + INSTR_SIZE;
            break;
        }
        case OP_JGE: {
            uint16_t target = mem_read_word(cpu->mem, cpu->pc);
            cpu->pc = !(cpu->flags & FLAG_N) ? target : cpu->pc + INSTR_SIZE;
            break;
        }
        case OP_JC: {
            uint16_t target = mem_read_word(cpu->mem, cpu->pc);
            cpu->pc = (cpu->flags & FLAG_C) ? target : cpu->pc + INSTR_SIZE;
            break;
        }

        /* ── Subroutine ────────────────────────────────── */
        case OP_CALL: {
            uint16_t target = mem_read_word(cpu->mem, cpu->pc);
            cpu->pc += INSTR_SIZE;     /* advance past the address word */
            cpu_push(cpu, cpu->pc);    /* push return address            */
            cpu->pc = target;
            break;
        }
        case OP_RET:
            cpu->pc = cpu_pop(cpu);
            break;

        /* ── I/O ───────────────────────────────────────── */
        case OP_IN: {
            uint16_t port = mem_read_word(cpu->mem, cpu->pc);
            cpu->pc += INSTR_SIZE;
            cpu->reg[dst] = mmio_read(cpu->mem, port);
            break;
        }
        case OP_OUT: {
            uint16_t port = mem_read_word(cpu->mem, cpu->pc);
            cpu->pc += INSTR_SIZE;
            mmio_write(cpu->mem, port, cpu->reg[dst]);
            break;
        }

        default:
            fprintf(stderr, "[CPU] Unknown opcode 0x%02X at PC=0x%04X\n",
                    opcode, cpu->pc - INSTR_SIZE);
            cpu->halted = 1;
            break;
    }
}

/* ============================================================
   STEP — one full fetch-decode-execute cycle
   ============================================================ */
int cpu_step(CPU *cpu) {
    if (cpu->halted) return 1;

    /* Tick timer if running */
    if (cpu->mem->timer_running)
        cpu->mem->timer_count++;

    uint16_t instr = cpu_fetch(cpu);
    cpu_execute(cpu, instr);
    cpu->cycle_count++;

    return cpu->halted ? 1 : 0;
}

/* ============================================================
   RUN — loop until halted
   ============================================================ */
uint64_t cpu_run(CPU *cpu) {
    while (!cpu->halted)
        cpu_step(cpu);
    return cpu->cycle_count;
}

/* ============================================================
   Debug
   ============================================================ */
void cpu_dump(CPU *cpu) {
    printf("\n=== CPU State (cycle %llu) ===\n",
           (unsigned long long)cpu->cycle_count);
    printf("  PC=0x%04X  SP=0x%04X\n", cpu->pc, cpu->sp);
    for (int i = 0; i < NUM_GP_REGS; i++)
        printf("  %s=0x%04X (%5d)  ", reg_names[i], cpu->reg[i], cpu->reg[i]);
    printf("\n");
    printf("  FLAGS: Z=%d N=%d C=%d V=%d\n",
           (cpu->flags & FLAG_Z) != 0,
           (cpu->flags & FLAG_N) != 0,
           (cpu->flags & FLAG_C) != 0,
           (cpu->flags & FLAG_V) != 0);
}

void cpu_print_instr(uint16_t instr, uint16_t pc) {
    uint8_t opcode = INSTR_OPCODE(instr);
    uint8_t mode   = INSTR_MODE(instr);
    uint8_t dst    = INSTR_DST(instr);
    uint8_t src    = INSTR_SRC(instr);

    const char *mname = (opcode < NUM_OPCODES) ? opcode_names[opcode] : "???";
    printf("0x%04X: [0x%04X]  %-6s  dst=%s  mode=%u  src=0x%02X\n",
           pc, instr, mname, reg_names[dst & 0x3], mode, src);
}

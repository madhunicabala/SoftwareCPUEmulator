#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "memory.h"
#include "isa.h"

static int tests_run = 0, tests_passed = 0;

#define ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { tests_passed++; printf("  PASS  %s\n", msg); } \
    else       { printf("  FAIL  %s  (%s:%d)\n", msg, __FILE__, __LINE__); } \
} while(0)

/* Helpers */
static void make_cpu(CPU *cpu, Memory *mem) {
    mem_init(mem);
    cpu_init(cpu, mem);
}

static void write_instr(CPU *cpu, uint16_t instr) {
    mem_write_word(cpu->mem, cpu->pc, instr);
}

/* ── Lifecycle ─────────────────────────────────────────────── */

static void test_init(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    ASSERT(cpu.pc    == CODE_SEG_BASE,  "init: PC = CODE_SEG_BASE");
    ASSERT(cpu.sp    == STACK_INIT_SP,  "init: SP = STACK_INIT_SP");
    ASSERT(cpu.flags == 0,              "init: FLAGS = 0");
    ASSERT(cpu.halted == 0,             "init: halted = 0");
    ASSERT(cpu.cycle_count == 0,        "init: cycle_count = 0");
    for (int i = 0; i < NUM_GP_REGS; i++)
        ASSERT(cpu.reg[i] == 0, "init: general register = 0");
}

static void test_reset(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    cpu.reg[0] = 0x1234;
    cpu.flags  = 0xFF;
    cpu.pc     = 0x3000;
    cpu.halted = 1;
    cpu_reset(&cpu);

    ASSERT(cpu.pc    == CODE_SEG_BASE, "reset: PC back to CODE_SEG_BASE");
    ASSERT(cpu.sp    == STACK_INIT_SP, "reset: SP back to STACK_INIT_SP");
    ASSERT(cpu.flags == 0,             "reset: FLAGS cleared");
    ASSERT(cpu.halted == 0,            "reset: halted cleared");
    ASSERT(cpu.reg[0] == 0,            "reset: R0 cleared");
}

/* ── Stack ──────────────────────────────────────────────────── */

static void test_stack(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);
    uint16_t old_sp = cpu.sp;

    cpu_push(&cpu, 0x1234);
    ASSERT(cpu.sp == old_sp - 2,                      "push: SP -= 2");
    ASSERT(mem_read_word(cpu.mem, cpu.sp) == 0x1234,  "push: value stored at new SP");

    uint16_t v = cpu_pop(&cpu);
    ASSERT(v    == 0x1234,  "pop: correct value returned");
    ASSERT(cpu.sp == old_sp, "pop: SP restored");

    /* LIFO order */
    cpu_push(&cpu, 0xAAAA);
    cpu_push(&cpu, 0xBBBB);
    ASSERT(cpu_pop(&cpu) == 0xBBBB, "stack LIFO: second push popped first");
    ASSERT(cpu_pop(&cpu) == 0xAAAA, "stack LIFO: first push popped second");
}

/* ── Flags ──────────────────────────────────────────────────── */

static void test_flags(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    cpu_set_flags(&cpu, 0, 0, 5, 5, 1);
    ASSERT(  cpu.flags & FLAG_Z, "flags: zero result → Z set");
    ASSERT(!(cpu.flags & FLAG_N), "flags: zero result → N clear");

    cpu_set_flags(&cpu, 0x8001, 0x8001, 10, 20, 1);
    ASSERT(  cpu.flags & FLAG_N, "flags: MSB set → N set");
    ASSERT(!(cpu.flags & FLAG_Z), "flags: nonzero → Z clear");

    cpu_set_flags(&cpu, 0x0001, 0x10001u, 0xFFFF, 2, 0);
    ASSERT(cpu.flags & FLAG_C, "flags: full > 0xFFFF → C set");

    cpu_set_flags(&cpu, 0x0005, 0x0005u, 3, 2, 0);
    ASSERT(!(cpu.flags & FLAG_C), "flags: no overflow → C clear");
}

/* ── Instruction execution ──────────────────────────────────── */

static void test_nop_halt(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    write_instr(&cpu, MAKE_INSTR(OP_NOP, MODE_REG, 0, 0));
    cpu_step(&cpu);
    ASSERT(!cpu.halted,                       "NOP: not halted");
    ASSERT(cpu.pc == CODE_SEG_BASE + 2,       "NOP: PC += 2");

    write_instr(&cpu, MAKE_INSTR(OP_HALT, MODE_REG, 0, 0));
    cpu_step(&cpu);
    ASSERT(cpu.halted, "HALT: halted flag set");
}

static void test_mov(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    /* MOV R0, #7 (IMM) */
    write_instr(&cpu, MAKE_INSTR(OP_MOV, MODE_IMM, REG_R0, 7));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R0] == 7, "MOV R0,#7 → R0=7");

    /* MOV R1, R0 (REG) */
    cpu.reg[REG_R0] = 42;
    write_instr(&cpu, MAKE_INSTR(OP_MOV, MODE_REG, REG_R1, REG_R0));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R1] == 42, "MOV R1,R0 → R1=R0");

    /* MOV R2, #-1 (negative IMM, sign-extended) */
    write_instr(&cpu, MAKE_INSTR(OP_MOV, MODE_IMM, REG_R2, (uint8_t)(-1 & 0x3F)));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R2] == 0xFFFF, "MOV R2,#-1 → R2=0xFFFF (sign-extended)");
}

static void test_movw(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    /* MOVW R0, 0xABCD */
    mem_write_word(cpu.mem, cpu.pc,     MAKE_INSTR(OP_MOVW, MODE_DIRECT, REG_R0, 0));
    mem_write_word(cpu.mem, cpu.pc + 2, 0xABCD);
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R0] == 0xABCD,       "MOVW R0,0xABCD → R0=0xABCD");
    ASSERT(cpu.pc == CODE_SEG_BASE + 4,     "MOVW: PC advanced 4 bytes");
}

static void test_add(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    cpu.reg[REG_R0] = 10;
    cpu.reg[REG_R1] = 7;
    write_instr(&cpu, MAKE_INSTR(OP_ADD, MODE_REG, REG_R0, REG_R1));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R0] == 17,    "ADD R0,R1 → 10+7=17");
    ASSERT(!(cpu.flags & FLAG_Z),    "ADD 10+7 → Z clear");
    ASSERT(!(cpu.flags & FLAG_C),    "ADD 10+7 → C clear");

    /* ADD overflow → carry */
    cpu.reg[REG_R0] = 0xFFFF;
    write_instr(&cpu, MAKE_INSTR(OP_ADD, MODE_IMM, REG_R0, 1));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R0] == 0,    "ADD 0xFFFF+1 → 0 (wrap)");
    ASSERT(cpu.flags & FLAG_Z,      "ADD 0xFFFF+1 → Z set");
    ASSERT(cpu.flags & FLAG_C,      "ADD 0xFFFF+1 → C set");
}

static void test_sub(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    cpu.reg[REG_R0] = 10;
    write_instr(&cpu, MAKE_INSTR(OP_SUB, MODE_IMM, REG_R0, 3));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R0] == 7, "SUB R0,#3 → 10-3=7");

    /* SUB to zero → Z set */
    cpu.reg[REG_R0] = 5;
    write_instr(&cpu, MAKE_INSTR(OP_SUB, MODE_IMM, REG_R0, 5));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R0] == 0, "SUB R0,#5 → 5-5=0");
    ASSERT(cpu.flags & FLAG_Z,   "SUB to zero → Z set");
}

static void test_mul(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    cpu.reg[REG_R0] = 6;
    cpu.reg[REG_R1] = 7;
    write_instr(&cpu, MAKE_INSTR(OP_MUL, MODE_REG, REG_R0, REG_R1));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R0] == 42, "MUL R0,R1 → 6*7=42");
}

static void test_inc_dec(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    cpu.reg[REG_R2] = 9;
    write_instr(&cpu, MAKE_INSTR(OP_INC, MODE_REG, REG_R2, 0));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R2] == 10, "INC R2 → 9+1=10");

    write_instr(&cpu, MAKE_INSTR(OP_DEC, MODE_REG, REG_R2, 0));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R2] == 9, "DEC R2 → 10-1=9");

    /* DEC to zero → Z set */
    cpu.reg[REG_R2] = 1;
    write_instr(&cpu, MAKE_INSTR(OP_DEC, MODE_REG, REG_R2, 0));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R2] == 0, "DEC R2 → 1-1=0");
    ASSERT(cpu.flags & FLAG_Z,   "DEC to zero → Z set");
}

static void test_logic_ops(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    cpu.reg[REG_R0] = 0xFF00;
    cpu.reg[REG_R1] = 0x0FF0;
    write_instr(&cpu, MAKE_INSTR(OP_AND, MODE_REG, REG_R0, REG_R1));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R0] == 0x0F00, "AND R0,R1 → 0x0F00");

    cpu.reg[REG_R0] = 0xF0F0;
    cpu.reg[REG_R1] = 0x0F0F;
    write_instr(&cpu, MAKE_INSTR(OP_OR, MODE_REG, REG_R0, REG_R1));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R0] == 0xFFFF, "OR R0,R1 → 0xFFFF");

    cpu.reg[REG_R0] = 0xFFFF;
    write_instr(&cpu, MAKE_INSTR(OP_NOT, MODE_REG, REG_R0, 0));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R0] == 0x0000, "NOT R0 → ~0xFFFF=0");
    ASSERT(cpu.flags & FLAG_Z, "NOT → zero result → Z set");

    cpu.reg[REG_R0] = 0xAAAA;
    write_instr(&cpu, MAKE_INSTR(OP_SHL, MODE_IMM, REG_R0, 1));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R0] == 0x5554, "SHL R0,1 → 0xAAAA<<1=0x5554");

    cpu.reg[REG_R0] = 0x00F0;
    write_instr(&cpu, MAKE_INSTR(OP_SHR, MODE_IMM, REG_R0, 4));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R0] == 0x000F, "SHR R0,4 → 0x00F0>>4=0x000F");
}

static void test_cmp(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    /* Equal → Z set, register unchanged */
    cpu.reg[REG_R0] = 5;
    write_instr(&cpu, MAKE_INSTR(OP_CMP, MODE_IMM, REG_R0, 5));
    cpu_step(&cpu);
    ASSERT(  cpu.flags & FLAG_Z,     "CMP 5,5 → Z set");
    ASSERT(!(cpu.flags & FLAG_N),    "CMP 5,5 → N clear");
    ASSERT(  cpu.reg[REG_R0] == 5,  "CMP does not modify dst");

    /* Less than → N set */
    cpu.reg[REG_R0] = 3;
    write_instr(&cpu, MAKE_INSTR(OP_CMP, MODE_IMM, REG_R0, 10));
    cpu_step(&cpu);
    ASSERT(!(cpu.flags & FLAG_Z),   "CMP 3,10 → Z clear");
    ASSERT(  cpu.flags & FLAG_N,    "CMP 3,10 → N set");
}

static void test_jmp(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    mem_write_word(cpu.mem, cpu.pc,     MAKE_INSTR(OP_JMP, MODE_DIRECT, 0, 0));
    mem_write_word(cpu.mem, cpu.pc + 2, 0x2040);
    cpu_step(&cpu);
    ASSERT(cpu.pc == 0x2040, "JMP 0x2040 → PC=0x2040");
}

static void test_jz(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    /* JZ taken */
    cpu.flags = FLAG_Z;
    mem_write_word(cpu.mem, cpu.pc,     MAKE_INSTR(OP_JZ, MODE_DIRECT, 0, 0));
    mem_write_word(cpu.mem, cpu.pc + 2, 0x2080);
    cpu_step(&cpu);
    ASSERT(cpu.pc == 0x2080, "JZ taken: PC = target");

    /* JZ not taken */
    cpu_reset(&cpu);
    cpu.flags = 0;
    mem_write_word(cpu.mem, cpu.pc,     MAKE_INSTR(OP_JZ, MODE_DIRECT, 0, 0));
    mem_write_word(cpu.mem, cpu.pc + 2, 0x2080);
    cpu_step(&cpu);
    ASSERT(cpu.pc == CODE_SEG_BASE + 4, "JZ not taken: PC falls through +4");
}

static void test_jnz(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    /* JNZ taken (Z clear) */
    cpu.flags = 0;
    mem_write_word(cpu.mem, cpu.pc,     MAKE_INSTR(OP_JNZ, MODE_DIRECT, 0, 0));
    mem_write_word(cpu.mem, cpu.pc + 2, 0x20A0);
    cpu_step(&cpu);
    ASSERT(cpu.pc == 0x20A0, "JNZ taken (Z=0): PC = target");

    /* JNZ not taken (Z set) */
    cpu_reset(&cpu);
    cpu.flags = FLAG_Z;
    mem_write_word(cpu.mem, cpu.pc,     MAKE_INSTR(OP_JNZ, MODE_DIRECT, 0, 0));
    mem_write_word(cpu.mem, cpu.pc + 2, 0x20A0);
    cpu_step(&cpu);
    ASSERT(cpu.pc == CODE_SEG_BASE + 4, "JNZ not taken (Z=1): PC falls through");
}

static void test_call_ret(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);
    uint16_t ret_addr = CODE_SEG_BASE + 4; /* after 4-byte CALL */

    /* CALL 0x2100 */
    mem_write_word(cpu.mem, cpu.pc,     MAKE_INSTR(OP_CALL, MODE_DIRECT, 0, 0));
    mem_write_word(cpu.mem, cpu.pc + 2, 0x2100);
    cpu_step(&cpu);
    ASSERT(cpu.pc == 0x2100, "CALL → PC = target");

    /* RET */
    mem_write_word(cpu.mem, 0x2100, MAKE_INSTR(OP_RET, MODE_REG, 0, 0));
    cpu_step(&cpu);
    ASSERT(cpu.pc == ret_addr, "RET → PC = return address");
}

static void test_push_pop_instr(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    cpu.reg[REG_R1] = 0xBEEF;
    write_instr(&cpu, MAKE_INSTR(OP_PUSH, MODE_REG, REG_R1, 0));
    cpu_step(&cpu);

    write_instr(&cpu, MAKE_INSTR(OP_POP, MODE_REG, REG_R2, 0));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R2] == 0xBEEF, "PUSH R1 / POP R2 → R2=0xBEEF");
    ASSERT(cpu.sp == STACK_INIT_SP,    "PUSH+POP: SP restored");
}

static void test_load_store(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    mem_write_word(cpu.mem, 0x0050, 0xCAFE);

    /* LOAD R0, [0x0050] */
    mem_write_word(cpu.mem, cpu.pc,     MAKE_INSTR(OP_LOAD, MODE_DIRECT, REG_R0, 0));
    mem_write_word(cpu.mem, cpu.pc + 2, 0x0050);
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R0] == 0xCAFE, "LOAD R0,[0x0050] → R0=0xCAFE");

    /* STORE R0, [0x0060] */
    cpu.reg[REG_R0] = 0x1234;
    mem_write_word(cpu.mem, cpu.pc,     MAKE_INSTR(OP_STORE, MODE_DIRECT, REG_R0, 0));
    mem_write_word(cpu.mem, cpu.pc + 2, 0x0060);
    cpu_step(&cpu);
    ASSERT(mem_read_word(cpu.mem, 0x0060) == 0x1234, "STORE R0,[0x0060] → mem=0x1234");

    /* LOAD R1, [R0] (indirect) — R0 holds address */
    cpu.reg[REG_R0] = 0x0060;
    write_instr(&cpu, MAKE_INSTR(OP_LOAD, MODE_INDIRECT, REG_R1, REG_R0));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R1] == 0x1234, "LOAD R1,[R0] indirect → R1=0x1234");
}

static void test_loadb(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    mem_write_byte(cpu.mem, 0x0070, 0xAB);
    cpu.reg[REG_R1] = 0x0070;

    write_instr(&cpu, MAKE_INSTR(OP_LOADB, MODE_INDIRECT, REG_R0, REG_R1));
    cpu_step(&cpu);
    ASSERT(cpu.reg[REG_R0] == 0x00AB, "LOADB R0,[R1] → R0=0x00AB");
}

/* ── Full program run ─────────────────────────────────────── */

static void test_cpu_run(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    /* Program: MOV R0,#7  MOV R1,#3  ADD R0,R1  HALT */
    uint16_t prog[] = {
        MAKE_INSTR(OP_MOV, MODE_IMM, REG_R0, 7),
        MAKE_INSTR(OP_MOV, MODE_IMM, REG_R1, 3),
        MAKE_INSTR(OP_ADD, MODE_REG, REG_R0, REG_R1),
        MAKE_INSTR(OP_HALT, MODE_REG, 0, 0),
    };
    int rc = cpu_load(&cpu, (uint8_t *)prog, sizeof(prog));
    ASSERT(rc == 0, "cpu_load: returns 0 on success");

    cpu_run(&cpu);
    ASSERT(cpu.reg[REG_R0] == 10, "cpu_run: MOV+ADD: R0=7+3=10");
    ASSERT(cpu.halted,            "cpu_run: HALT reached");
}

static void test_cpu_run_loop(void) {
    CPU cpu; Memory mem;
    make_cpu(&cpu, &mem);

    /* Program: count R0 from 0 to 3 using INC + CMP + JNZ + HALT
       0x2000: MOV R0,#0
       0x2002: INC R0
       0x2004: CMP R0,#3
       0x2006: JNZ 0x2002   (4 bytes: instr + addr)
       0x200A: HALT
    */
    uint16_t prog[] = {
        MAKE_INSTR(OP_MOV, MODE_IMM, REG_R0, 0),   /* 0x2000 */
        MAKE_INSTR(OP_INC, MODE_REG, REG_R0, 0),    /* 0x2002 */
        MAKE_INSTR(OP_CMP, MODE_IMM, REG_R0, 3),    /* 0x2004 */
        MAKE_INSTR(OP_JNZ, MODE_DIRECT, 0, 0),      /* 0x2006 */
        0x2002,                                      /* 0x2008  target addr */
        MAKE_INSTR(OP_HALT, MODE_REG, 0, 0),        /* 0x200A */
    };
    cpu_load(&cpu, (uint8_t *)prog, sizeof(prog));
    cpu_run(&cpu);
    ASSERT(cpu.reg[REG_R0] == 3, "loop: R0 counted to 3");
    ASSERT(cpu.halted,           "loop: HALT reached");
}

int main(void) {
    printf("=== test_cpu ===\n");
    test_init();
    test_reset();
    test_stack();
    test_flags();
    test_nop_halt();
    test_mov();
    test_movw();
    test_add();
    test_sub();
    test_mul();
    test_inc_dec();
    test_logic_ops();
    test_cmp();
    test_jmp();
    test_jz();
    test_jnz();
    test_call_ret();
    test_push_pop_instr();
    test_load_store();
    test_loadb();
    test_cpu_run();
    test_cpu_run_loop();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}

#include <stdio.h>
#include <string.h>
#include "trace.h"
#include "cpu.h"
#include "memory.h"
#include "isa.h"

static const char *op_name(uint8_t op) {
    static const char *names[] = {
        "NOP","HALT","MOV","LOAD","STORE","PUSH","POP",
        "ADD","SUB","MUL","INC","DEC",
        "AND","OR","XOR","NOT","SHL","SHR",
        "CMP",
        "JMP","JZ","JNZ","JL","JGE","JC",
        "CALL","RET",
        "IN","OUT",
        "LOADB","MOVW"
    };
    return (op < NUM_OPCODES) ? names[op] : "???";
}

void trace_open(TraceWriter *tw, const char *path) {
    tw->f     = NULL;
    tw->first = 1;
    if (!path) return;
    tw->f = fopen(path, "w");
    if (tw->f) fprintf(tw->f, "[\n");
}

void trace_record(TraceWriter *tw, const CPU *cpu,
                  uint16_t pc_before, uint16_t instr) {
    if (!tw->f) return;

    uint8_t op   = INSTR_OPCODE(instr);
    uint8_t mode = INSTR_MODE(instr);
    uint8_t dst  = INSTR_DST(instr);
    uint8_t src  = INSTR_SRC(instr);

    if (!tw->first) fprintf(tw->f, ",\n");
    tw->first = 0;

    fprintf(tw->f,
        "  {"
        "\"cycle\":%llu,"
        "\"pc\":\"0x%04X\","
        "\"instr\":\"0x%04X\","
        "\"op\":\"%s\","
        "\"mode\":%u,"
        "\"dst\":%u,"
        "\"src\":%u,"
        "\"reg\":[%u,%u,%u,%u],"
        "\"sp\":\"0x%04X\","
        "\"flags\":{\"Z\":%d,\"N\":%d,\"C\":%d,\"V\":%d}"
        "}",
        (unsigned long long)cpu->cycle_count,
        pc_before,
        instr,
        op_name(op),
        mode, dst, src,
        cpu->reg[0], cpu->reg[1], cpu->reg[2], cpu->reg[3],
        cpu->sp,
        (cpu->flags & FLAG_Z) ? 1 : 0,
        (cpu->flags & FLAG_N) ? 1 : 0,
        (cpu->flags & FLAG_C) ? 1 : 0,
        (cpu->flags & FLAG_V) ? 1 : 0
    );
}

int trace_step(TraceWriter *tw, CPU *cpu) {
    uint16_t pc_before = cpu->pc;
    uint16_t instr     = mem_read_word(cpu->mem, pc_before);
    int      done      = cpu_step(cpu);
    trace_record(tw, cpu, pc_before, instr);
    return done;
}

void trace_close(TraceWriter *tw) {
    if (!tw->f) return;
    fprintf(tw->f, "\n]\n");
    fclose(tw->f);
    tw->f = NULL;
}

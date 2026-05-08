#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "memory.h"
#include "isa.h"
#include "trace.h"

/* ============================================================
   Load a binary .bin file into a buffer
   ============================================================ */
static uint8_t *load_binary(const char *path, uint32_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open '%s'\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    uint8_t *buf = (uint8_t *)malloc(size);
    if (!buf) { fclose(f); return NULL; }

    fread(buf, 1, size, f);
    fclose(f);
    *out_size = (uint32_t)size;
    return buf;
}

/* ============================================================
   Usage
   ============================================================ */
static void print_usage(const char *argv0) {
    printf("Usage:\n");
    printf("  %s run   <program.bin>               Run a binary program\n", argv0);
    printf("  %s debug <program.bin>               Step-by-step debug mode\n", argv0);
    printf("  %s dump  <program.bin>               Dump memory after run\n", argv0);
    printf("  %s trace <program.bin> [trace.json]  Run and record execution trace\n", argv0);
}

/* ============================================================
   Main
   ============================================================ */
int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char *cmd  = argv[1];
    const char *path = argv[2];

    /* Load program */
    uint32_t prog_size = 0;
    uint8_t *program   = load_binary(path, &prog_size);
    if (!program) return 1;

    /* Initialise hardware */
    Memory mem;
    CPU    cpu;
    mem_init(&mem);
    cpu_init(&cpu, &mem);

    if (cpu_load(&cpu, program, prog_size) != 0) {
        free(program);
        return 1;
    }
    free(program);

    /* Execute */
    if (strcmp(cmd, "run") == 0) {
        uint64_t cycles = cpu_run(&cpu);
        printf("[CPU] Finished in %llu cycles\n", (unsigned long long)cycles);
        cpu_dump(&cpu);

    } else if (strcmp(cmd, "debug") == 0) {
        printf("[DEBUG] Stepping. Press Enter to advance, 'q' to quit.\n");
        char line[16];
        while (!cpu.halted) {
            cpu_print_instr(mem_read_word(&mem, cpu.pc), cpu.pc);
            cpu_dump(&cpu);
            printf("> ");
            if (!fgets(line, sizeof(line), stdin)) break;
            if (line[0] == 'q') break;
            cpu_step(&cpu);
        }

    } else if (strcmp(cmd, "dump") == 0) {
        cpu_run(&cpu);
        mem_dump(&mem, DATA_SEG_BASE, DATA_SEG_BASE + 0xFF);
        mem_dump(&mem, STACK_SEG_BASE, STACK_SEG_END);
        cpu_dump(&cpu);

    } else if (strcmp(cmd, "trace") == 0) {
        /* Derive trace path: argv[3] if given, else replace .bin → .trace.json */
        char trace_path[512];
        if (argc >= 4) {
            strncpy(trace_path, argv[3], sizeof(trace_path) - 1);
            trace_path[sizeof(trace_path) - 1] = '\0';
        } else {
            strncpy(trace_path, path, sizeof(trace_path) - 20);
            trace_path[sizeof(trace_path) - 20] = '\0';
            char *dot = strrchr(trace_path, '.');
            if (dot && strcmp(dot, ".bin") == 0) *dot = '\0';
            strcat(trace_path, ".trace.json");
        }

        TraceWriter tw;
        trace_open(&tw, trace_path);
        while (!cpu.halted)
            trace_step(&tw, &cpu);
        trace_close(&tw);

        printf("[CPU] Finished in %llu cycles\n",
               (unsigned long long)cpu.cycle_count);
        printf("[TRACE] Written to %s\n", trace_path);
        cpu_dump(&cpu);

    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}

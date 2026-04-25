#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include "isa.h"
#include "memory.h"

/* ------------------------------------------------------------
   CPU State
   ------------------------------------------------------------ */
typedef struct {
    /* General-purpose registers R0–R3 */
    uint16_t reg[NUM_GP_REGS];

    /* Special registers */
    uint16_t pc;    /* Program Counter — address of next instruction */
    uint16_t sp;    /* Stack Pointer   — top of stack                */
    uint8_t  flags; /* Flag register   — Z N C V in bits 0–3         */

    /* Linked memory */
    Memory  *mem;

    /* Execution state */
    uint8_t  halted;      /* 1 = HALT was executed              */
    uint64_t cycle_count; /* total cycles executed              */
} CPU;

/* ------------------------------------------------------------
   CPU lifecycle
   ------------------------------------------------------------ */
void cpu_init(CPU *cpu, Memory *mem);
void cpu_reset(CPU *cpu);

/* Load binary program into code segment */
int  cpu_load(CPU *cpu, const uint8_t *program, uint32_t size);

/* Run until HALT or error; returns cycle count */
uint64_t cpu_run(CPU *cpu);

/* Execute a single fetch-decode-execute cycle; returns 0=ok 1=halted */
int  cpu_step(CPU *cpu);

/* ------------------------------------------------------------
   Fetch / Decode / Execute (internal — exposed for testing)
   ------------------------------------------------------------ */
uint16_t cpu_fetch(CPU *cpu);
void     cpu_execute(CPU *cpu, uint16_t instr);

/* ------------------------------------------------------------
   Flag helpers
   ------------------------------------------------------------ */
void cpu_set_flags(CPU *cpu, uint16_t result, uint32_t full_result,
                   uint16_t a, uint16_t b, int is_sub);

/* ------------------------------------------------------------
   Stack helpers
   ------------------------------------------------------------ */
void     cpu_push(CPU *cpu, uint16_t value);
uint16_t cpu_pop(CPU *cpu);

/* ------------------------------------------------------------
   Debug
   ------------------------------------------------------------ */
void cpu_dump(CPU *cpu);
void cpu_print_instr(uint16_t instr, uint16_t pc);

#endif /* CPU_H */

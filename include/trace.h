#ifndef TRACE_H
#define TRACE_H

#include <stdio.h>
#include <stdint.h>
#include "cpu.h"

/* ------------------------------------------------------------
   TraceWriter — manages one open trace file
   ------------------------------------------------------------ */
typedef struct {
    FILE *f;
    int   first;   /* 1 until the first record has been written */
} TraceWriter;

/* Open trace file and write opening '['.
   No-op (silently) if path is NULL or fopen fails. */
void trace_open  (TraceWriter *tw, const char *path);

/* Append one JSON record for the cycle that just executed.
   pc_before — PC value before fetch
   instr     — 16-bit instruction word that was fetched */
void trace_record(TraceWriter *tw, const CPU *cpu,
                  uint16_t pc_before, uint16_t instr);

/* Convenience wrapper: capture pc/instr, call cpu_step, then record.
   Returns same value as cpu_step (0 = running, 1 = halted). */
int  trace_step  (TraceWriter *tw, CPU *cpu);

/* Write closing ']' and close file. */
void trace_close (TraceWriter *tw);

#endif /* TRACE_H */

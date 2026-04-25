#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include "isa.h"

/* ------------------------------------------------------------
   Memory subsystem
   ------------------------------------------------------------ */
typedef struct {
    uint8_t  data[MEM_SIZE];   /* flat 64KB byte array           */

    /* MMIO state */
    uint32_t timer_count;      /* increments each CPU cycle      */
    uint8_t  timer_running;    /* 1 = timer active               */
} Memory;

/* Lifecycle */
void     mem_init(Memory *mem);

/* Byte access */
uint8_t  mem_read_byte(Memory *mem, uint16_t addr);
void     mem_write_byte(Memory *mem, uint16_t addr, uint8_t value);

/* Word (16-bit) access — little-endian */
uint16_t mem_read_word(Memory *mem, uint16_t addr);
void     mem_write_word(Memory *mem, uint16_t addr, uint16_t value);

/* MMIO handlers (called internally by read/write) */
uint16_t mmio_read(Memory *mem, uint16_t addr);
void     mmio_write(Memory *mem, uint16_t addr, uint16_t value);

/* Debug */
void     mem_dump(Memory *mem, uint16_t from, uint16_t to);

#endif /* MEMORY_H */

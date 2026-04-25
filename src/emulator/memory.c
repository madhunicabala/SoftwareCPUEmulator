#include <stdio.h>
#include <string.h>
#include "memory.h"

/* ============================================================
   Memory — initialise
   ============================================================ */
void mem_init(Memory *mem) {
    memset(mem->data, 0, MEM_SIZE);
    mem->timer_count   = 0;
    mem->timer_running = 0;
}

/* ============================================================
   MMIO handlers
   ============================================================ */
uint16_t mmio_read(Memory *mem, uint16_t addr) {
    switch (addr) {
        case MMIO_TIMER_COUNT:
            return (uint16_t)(mem->timer_count & 0xFFFF);
        case MMIO_TIMER_CTRL:
            return mem->timer_running;
        case MMIO_STDIN: {
            /* Read one character from stdin */
            int c = getchar();
            return (c == EOF) ? 0 : (uint16_t)c;
        }
        default:
            fprintf(stderr, "[MEM] MMIO read from unknown port 0x%04X\n", addr);
            return 0;
    }
}

void mmio_write(Memory *mem, uint16_t addr, uint16_t value) {
    switch (addr) {
        case MMIO_TIMER_CTRL:
            mem->timer_running = (value != 0) ? 1 : 0;
            if (value == 0) mem->timer_count = 0; /* reset on stop */
            break;
        case MMIO_STDOUT:
            /* Write character to stdout */
            putchar((char)(value & 0xFF));
            fflush(stdout);
            break;
        default:
            /* For other MMIO addresses just store in data array */
            mem->data[addr]     = (uint8_t)(value & 0xFF);
            mem->data[addr + 1] = (uint8_t)((value >> 8) & 0xFF);
            break;
    }
}

/* ============================================================
   Byte read/write
   ============================================================ */
uint8_t mem_read_byte(Memory *mem, uint16_t addr) {
    if (addr >= MMIO_BASE && addr <= MMIO_END)
        return (uint8_t)(mmio_read(mem, addr) & 0xFF);
    return mem->data[addr];
}

void mem_write_byte(Memory *mem, uint16_t addr, uint8_t value) {
    if (addr >= MMIO_BASE && addr <= MMIO_END) {
        mmio_write(mem, addr, value);
        return;
    }
    mem->data[addr] = value;
}

/* ============================================================
   Word (16-bit) read/write — little-endian
   ============================================================ */
uint16_t mem_read_word(Memory *mem, uint16_t addr) {
    if (addr >= MMIO_BASE && addr <= MMIO_END)
        return mmio_read(mem, addr);
    /* little-endian: low byte at addr, high byte at addr+1 */
    return (uint16_t)(mem->data[addr] | (mem->data[addr + 1] << 8));
}

void mem_write_word(Memory *mem, uint16_t addr, uint16_t value) {
    if (addr >= MMIO_BASE && addr <= MMIO_END) {
        mmio_write(mem, addr, value);
        return;
    }
    mem->data[addr]     = (uint8_t)(value & 0xFF);
    mem->data[addr + 1] = (uint8_t)((value >> 8) & 0xFF);
}

/* ============================================================
   Debug dump
   ============================================================ */
void mem_dump(Memory *mem, uint16_t from, uint16_t to) {
    printf("\n=== Memory Dump [0x%04X – 0x%04X] ===\n", from, to);
    for (uint16_t addr = from; addr <= to; addr += 16) {
        printf("0x%04X: ", addr);
        for (int i = 0; i < 16 && (addr + i) <= to; i++)
            printf("%02X ", mem->data[addr + i]);
        printf("\n");
    }
}

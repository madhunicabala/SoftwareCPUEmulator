#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "isa.h"

static int tests_run = 0, tests_passed = 0;

#define ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { tests_passed++; printf("  PASS  %s\n", msg); } \
    else       { printf("  FAIL  %s  (%s:%d)\n", msg, __FILE__, __LINE__); } \
} while(0)

static void test_init(void) {
    Memory mem;
    mem_init(&mem);

    ASSERT(mem.timer_count   == 0, "init: timer_count = 0");
    ASSERT(mem.timer_running == 0, "init: timer_running = 0");
    ASSERT(mem.data[0x0000]  == 0, "init: data[0x0000] = 0");
    ASSERT(mem.data[0x1000]  == 0, "init: data[0x1000] = 0");
    ASSERT(mem.data[0x2000]  == 0, "init: data[0x2000] = 0");
    ASSERT(mem.data[0xF000]  == 0, "init: data[0xF000] = 0");
}

static void test_byte_rw(void) {
    Memory mem;
    mem_init(&mem);

    mem_write_byte(&mem, 0x0010, 0xAB);
    ASSERT(mem_read_byte(&mem, 0x0010) == 0xAB, "byte rw: 0xAB round-trip");

    mem_write_byte(&mem, 0x0000, 0xFF);
    ASSERT(mem_read_byte(&mem, 0x0000) == 0xFF, "byte rw: 0xFF at addr 0");

    mem_write_byte(&mem, 0x1FFE, 0x42);
    ASSERT(mem_read_byte(&mem, 0x1FFE) == 0x42, "byte rw: stack segment");

    /* Adjacent bytes are independent */
    mem_write_byte(&mem, 0x0100, 0x11);
    mem_write_byte(&mem, 0x0101, 0x22);
    ASSERT(mem_read_byte(&mem, 0x0100) == 0x11, "byte rw: adjacent lo independent");
    ASSERT(mem_read_byte(&mem, 0x0101) == 0x22, "byte rw: adjacent hi independent");

    /* Overwrite */
    mem_write_byte(&mem, 0x0010, 0xCD);
    ASSERT(mem_read_byte(&mem, 0x0010) == 0xCD, "byte rw: overwrite");

    /* Zero */
    mem_write_byte(&mem, 0x0200, 0x00);
    ASSERT(mem_read_byte(&mem, 0x0200) == 0x00, "byte rw: zero value");
}

static void test_word_rw(void) {
    Memory mem;
    mem_init(&mem);

    mem_write_word(&mem, 0x0010, 0x1234);
    ASSERT(mem_read_word(&mem, 0x0010) == 0x1234, "word rw: 0x1234 round-trip");

    /* Verify little-endian layout */
    ASSERT(mem.data[0x0010] == 0x34, "word rw: low byte at low addr (little-endian)");
    ASSERT(mem.data[0x0011] == 0x12, "word rw: high byte at high addr (little-endian)");

    mem_write_word(&mem, 0x0020, 0x0000);
    ASSERT(mem_read_word(&mem, 0x0020) == 0x0000, "word rw: zero");

    mem_write_word(&mem, 0x0022, 0xFFFF);
    ASSERT(mem_read_word(&mem, 0x0022) == 0xFFFF, "word rw: 0xFFFF");

    /* Adjacent words are independent */
    mem_write_word(&mem, 0x0030, 0xABCD);
    mem_write_word(&mem, 0x0032, 0xEF01);
    ASSERT(mem_read_word(&mem, 0x0030) == 0xABCD, "word rw: adjacent lo independent");
    ASSERT(mem_read_word(&mem, 0x0032) == 0xEF01, "word rw: adjacent hi independent");
}

static void test_byte_word_interop(void) {
    Memory mem;
    mem_init(&mem);

    /* Write word, read individual bytes */
    mem_write_word(&mem, 0x0040, 0xBEEF);
    ASSERT(mem_read_byte(&mem, 0x0040) == 0xEF, "interop: low byte of 0xBEEF = 0xEF");
    ASSERT(mem_read_byte(&mem, 0x0041) == 0xBE, "interop: high byte of 0xBEEF = 0xBE");

    /* Write bytes, read as word */
    mem_write_byte(&mem, 0x0050, 0x34);
    mem_write_byte(&mem, 0x0051, 0x12);
    ASSERT(mem_read_word(&mem, 0x0050) == 0x1234, "interop: bytes assembled to word");
}

static void test_mmio_timer(void) {
    Memory mem;
    mem_init(&mem);

    /* Write 1 to TIMER_CTRL starts the timer */
    mmio_write(&mem, MMIO_TIMER_CTRL, 1);
    ASSERT(mem.timer_running == 1, "mmio: write 1 to TIMER_CTRL starts timer");

    /* Write 0 stops and resets */
    mem.timer_count = 42;
    mmio_write(&mem, MMIO_TIMER_CTRL, 0);
    ASSERT(mem.timer_running == 0, "mmio: write 0 stops timer");
    ASSERT(mem.timer_count   == 0, "mmio: write 0 resets count to 0");

    /* Read TIMER_COUNT */
    mem.timer_count = 99;
    ASSERT(mmio_read(&mem, MMIO_TIMER_COUNT) == 99, "mmio: TIMER_COUNT returns correct value");

    /* Timer ticks don't happen without cpu_step; just verify count passthrough */
    mem.timer_count = 0xABCD;
    ASSERT((uint32_t)mmio_read(&mem, MMIO_TIMER_COUNT) == 0xABCD,
           "mmio: TIMER_COUNT passthrough large value");
}

static void test_mmio_stdout(void) {
    Memory mem;
    mem_init(&mem);

    /* Writing to STDOUT should not crash */
    mmio_write(&mem, MMIO_STDOUT, 'H');
    mmio_write(&mem, MMIO_STDOUT, '\n');
    ASSERT(1, "mmio: consecutive writes to STDOUT do not crash");
}

static void test_segments(void) {
    Memory mem;
    mem_init(&mem);

    /* Data segment */
    mem_write_word(&mem, DATA_SEG_BASE, 0x0001);
    ASSERT(mem_read_word(&mem, DATA_SEG_BASE) == 0x0001, "segment: data base r/w");

    /* Stack segment */
    mem_write_word(&mem, STACK_INIT_SP - 1, 0xCAFE);
    ASSERT(mem_read_word(&mem, STACK_INIT_SP - 1) == 0xCAFE, "segment: stack top r/w");

    /* Code segment */
    mem_write_word(&mem, CODE_SEG_BASE, 0x0401);
    ASSERT(mem_read_word(&mem, CODE_SEG_BASE) == 0x0401, "segment: code base r/w");

    mem_write_word(&mem, CODE_SEG_BASE + 2, 0xDEAD);
    ASSERT(mem_read_word(&mem, CODE_SEG_BASE + 2) == 0xDEAD, "segment: code +2 r/w");

    /* Writes to different segments don't alias */
    mem_write_word(&mem, DATA_SEG_BASE, 0x1111);
    mem_write_word(&mem, STACK_SEG_BASE, 0x2222);
    mem_write_word(&mem, CODE_SEG_BASE, 0x3333);
    ASSERT(mem_read_word(&mem, DATA_SEG_BASE)  == 0x1111, "segment: no alias data");
    ASSERT(mem_read_word(&mem, STACK_SEG_BASE) == 0x2222, "segment: no alias stack");
    ASSERT(mem_read_word(&mem, CODE_SEG_BASE)  == 0x3333, "segment: no alias code");
}

int main(void) {
    printf("=== test_memory ===\n");
    test_init();
    test_byte_rw();
    test_word_rw();
    test_byte_word_interop();
    test_mmio_timer();
    test_mmio_stdout();
    test_segments();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}

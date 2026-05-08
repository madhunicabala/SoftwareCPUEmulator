#include <stdio.h>
#include <stdint.h>
#include "alu.h"

static int tests_run = 0, tests_passed = 0;

#define ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { tests_passed++; printf("  PASS  %s\n", msg); } \
    else       { printf("  FAIL  %s  (%s:%d)\n", msg, __FILE__, __LINE__); } \
} while(0)

static void test_add(void) {
    ALUResult r;

    r = alu_add(10, 5);
    ASSERT(r.result == 15 && r.full == 15, "add: 10+5=15");

    r = alu_add(0, 0);
    ASSERT(r.result == 0, "add: 0+0=0");

    r = alu_add(0xFFFF, 1);
    ASSERT(r.result == 0 && r.full == 0x10000u, "add: 0xFFFF+1 wraps, carry in full");

    r = alu_add(0x7FFF, 0x7FFF);
    ASSERT(r.result == 0xFFFE, "add: 0x7FFF+0x7FFF=0xFFFE");

    r = alu_add(0x8000, 0x8000);
    ASSERT(r.result == 0 && r.full == 0x10000u, "add: 0x8000+0x8000 wraps");
}

static void test_sub(void) {
    ALUResult r;

    r = alu_sub(10, 3);
    ASSERT(r.result == 7, "sub: 10-3=7");

    r = alu_sub(5, 5);
    ASSERT(r.result == 0, "sub: 5-5=0");

    r = alu_sub(0, 1);
    ASSERT(r.result == 0xFFFF, "sub: 0-1 wraps to 0xFFFF");

    r = alu_sub(100, 50);
    ASSERT(r.result == 50, "sub: 100-50=50");

    r = alu_sub(0, 0);
    ASSERT(r.result == 0, "sub: 0-0=0");
}

static void test_mul(void) {
    ALUResult r;

    r = alu_mul(6, 7);
    ASSERT(r.result == 42, "mul: 6*7=42");

    r = alu_mul(0, 100);
    ASSERT(r.result == 0, "mul: 0*100=0");

    r = alu_mul(1, 0xFFFF);
    ASSERT(r.result == 0xFFFF, "mul: 1*0xFFFF=0xFFFF");

    r = alu_mul(0x100, 0x100);
    ASSERT(r.result == 0 && r.full == 0x10000u, "mul: 0x100*0x100=0x10000 (full carries)");

    r = alu_mul(12, 12);
    ASSERT(r.result == 144, "mul: 12*12=144");
}

static void test_inc(void) {
    ALUResult r;

    r = alu_inc(9);
    ASSERT(r.result == 10, "inc: 9->10");

    r = alu_inc(0);
    ASSERT(r.result == 1, "inc: 0->1");

    r = alu_inc(0xFFFF);
    ASSERT(r.result == 0 && r.full == 0x10000u, "inc: 0xFFFF wraps to 0");

    r = alu_inc(0x7FFF);
    ASSERT(r.result == 0x8000, "inc: 0x7FFF->0x8000");
}

static void test_dec(void) {
    ALUResult r;

    r = alu_dec(5);
    ASSERT(r.result == 4, "dec: 5->4");

    r = alu_dec(1);
    ASSERT(r.result == 0, "dec: 1->0");

    r = alu_dec(0);
    ASSERT(r.result == 0xFFFF, "dec: 0 wraps to 0xFFFF");
}

static void test_and(void) {
    ALUResult r;

    r = alu_and(0xFF00, 0x0FF0);
    ASSERT(r.result == 0x0F00, "and: 0xFF00 & 0x0FF0 = 0x0F00");

    r = alu_and(0xFFFF, 0x0000);
    ASSERT(r.result == 0, "and: 0xFFFF & 0 = 0");

    r = alu_and(0xFFFF, 0xFFFF);
    ASSERT(r.result == 0xFFFF, "and: 0xFFFF & 0xFFFF = 0xFFFF");

    r = alu_and(0xA5A5, 0x5A5A);
    ASSERT(r.result == 0, "and: 0xA5A5 & 0x5A5A = 0");
}

static void test_or(void) {
    ALUResult r;

    r = alu_or(0xF0F0, 0x0F0F);
    ASSERT(r.result == 0xFFFF, "or: 0xF0F0 | 0x0F0F = 0xFFFF");

    r = alu_or(0, 0);
    ASSERT(r.result == 0, "or: 0 | 0 = 0");

    r = alu_or(0x1234, 0);
    ASSERT(r.result == 0x1234, "or: 0x1234 | 0 = 0x1234");
}

static void test_xor(void) {
    ALUResult r;

    r = alu_xor(0xFFFF, 0xFFFF);
    ASSERT(r.result == 0, "xor: same values = 0");

    r = alu_xor(0xAAAA, 0x5555);
    ASSERT(r.result == 0xFFFF, "xor: alternating bits = 0xFFFF");

    r = alu_xor(0x1234, 0);
    ASSERT(r.result == 0x1234, "xor: value ^ 0 = value");
}

static void test_not(void) {
    ALUResult r;

    r = alu_not(0x0000);
    ASSERT(r.result == 0xFFFF, "not: ~0x0000 = 0xFFFF");

    r = alu_not(0xFFFF);
    ASSERT(r.result == 0x0000, "not: ~0xFFFF = 0x0000");

    r = alu_not(0xA5A5);
    ASSERT(r.result == 0x5A5A, "not: ~0xA5A5 = 0x5A5A");

    r = alu_not(0x00FF);
    ASSERT(r.result == 0xFF00, "not: ~0x00FF = 0xFF00");
}

static void test_shl(void) {
    ALUResult r;

    r = alu_shl(0x0001, 4);
    ASSERT(r.result == 0x0010, "shl: 1 << 4 = 0x10");

    r = alu_shl(0x0001, 15);
    ASSERT(r.result == 0x8000, "shl: 1 << 15 = 0x8000");

    r = alu_shl(0x0001, 16);
    ASSERT(r.result == 0 && r.full == 0x10000u, "shl: 1 << 16 overflows");

    r = alu_shl(0, 8);
    ASSERT(r.result == 0, "shl: 0 << 8 = 0");

    r = alu_shl(0x00FF, 4);
    ASSERT(r.result == 0x0FF0, "shl: 0x00FF << 4 = 0x0FF0");
}

static void test_shr(void) {
    ALUResult r;

    r = alu_shr(0x8000, 1);
    ASSERT(r.result == 0x4000, "shr: 0x8000 >> 1 = 0x4000");

    r = alu_shr(0x00FF, 4);
    ASSERT(r.result == 0x000F, "shr: 0x00FF >> 4 = 0x000F");

    r = alu_shr(0, 8);
    ASSERT(r.result == 0, "shr: 0 >> 8 = 0");

    r = alu_shr(0xFFFF, 8);
    ASSERT(r.result == 0x00FF, "shr: 0xFFFF >> 8 = 0x00FF");
}

static void test_cmp(void) {
    ALUResult r;

    r = alu_cmp(5, 5);
    ASSERT(r.result == 0, "cmp: equal → result=0");

    r = alu_cmp(10, 3);
    ASSERT(r.result == 7, "cmp: 10>3 → result=7 (same as sub)");

    r = alu_cmp(3, 10);
    ASSERT(r.result == (uint16_t)(3 - 10), "cmp: 3<10 → wraps (same as sub)");

    r = alu_cmp(0, 0);
    ASSERT(r.result == 0, "cmp: 0,0 → 0");
}

int main(void) {
    printf("=== test_alu ===\n");
    test_add();
    test_sub();
    test_mul();
    test_inc();
    test_dec();
    test_and();
    test_or();
    test_xor();
    test_not();
    test_shl();
    test_shr();
    test_cmp();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}

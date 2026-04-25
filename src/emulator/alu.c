#include "alu.h"

/* ============================================================
   Arithmetic
   ============================================================ */
ALUResult alu_add(uint16_t a, uint16_t b) {
    ALUResult r;
    r.full   = (uint32_t)a + (uint32_t)b;
    r.result = (uint16_t)(r.full & 0xFFFF);
    return r;
}

ALUResult alu_sub(uint16_t a, uint16_t b) {
    ALUResult r;
    r.full   = (uint32_t)a - (uint32_t)b;
    r.result = (uint16_t)(r.full & 0xFFFF);
    return r;
}

ALUResult alu_mul(uint16_t a, uint16_t b) {
    ALUResult r;
    r.full   = (uint32_t)a * (uint32_t)b;
    r.result = (uint16_t)(r.full & 0xFFFF);
    return r;
}

ALUResult alu_inc(uint16_t a) {
    return alu_add(a, 1);
}

ALUResult alu_dec(uint16_t a) {
    return alu_sub(a, 1);
}

/* ============================================================
   Logic
   ============================================================ */
ALUResult alu_and(uint16_t a, uint16_t b) {
    ALUResult r;
    r.result = a & b;
    r.full   = r.result;
    return r;
}

ALUResult alu_or(uint16_t a, uint16_t b) {
    ALUResult r;
    r.result = a | b;
    r.full   = r.result;
    return r;
}

ALUResult alu_xor(uint16_t a, uint16_t b) {
    ALUResult r;
    r.result = a ^ b;
    r.full   = r.result;
    return r;
}

ALUResult alu_not(uint16_t a) {
    ALUResult r;
    r.result = ~a;
    r.full   = r.result;
    return r;
}

ALUResult alu_shl(uint16_t a, uint8_t shift) {
    ALUResult r;
    r.full   = (uint32_t)a << shift;
    r.result = (uint16_t)(r.full & 0xFFFF);
    return r;
}

ALUResult alu_shr(uint16_t a, uint8_t shift) {
    ALUResult r;
    r.result = a >> shift;
    r.full   = r.result;
    return r;
}

/* ============================================================
   Compare — subtract but don't store result
   ============================================================ */
ALUResult alu_cmp(uint16_t a, uint16_t b) {
    return alu_sub(a, b);
}

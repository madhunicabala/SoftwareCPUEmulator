#ifndef ALU_H
#define ALU_H

#include <stdint.h>

/* ------------------------------------------------------------
   ALU result — carries both result and raw 32-bit value for
   flag computation (carry, overflow detection)
   ------------------------------------------------------------ */
typedef struct {
    uint16_t result;       /* 16-bit result stored in register   */
    uint32_t full;         /* 32-bit raw result for flag calc     */
} ALUResult;

/* Arithmetic */
ALUResult alu_add(uint16_t a, uint16_t b);
ALUResult alu_sub(uint16_t a, uint16_t b);
ALUResult alu_mul(uint16_t a, uint16_t b);
ALUResult alu_inc(uint16_t a);
ALUResult alu_dec(uint16_t a);

/* Logic */
ALUResult alu_and(uint16_t a, uint16_t b);
ALUResult alu_or (uint16_t a, uint16_t b);
ALUResult alu_xor(uint16_t a, uint16_t b);
ALUResult alu_not(uint16_t a);
ALUResult alu_shl(uint16_t a, uint8_t shift);
ALUResult alu_shr(uint16_t a, uint8_t shift);

/* Compare — same as SUB but result is discarded; only used for flags */
ALUResult alu_cmp(uint16_t a, uint16_t b);

#endif /* ALU_H */

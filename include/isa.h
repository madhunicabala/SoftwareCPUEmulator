#ifndef ISA_H
#define ISA_H

#include <stdint.h>

/* ============================================================
   SoftCPU-C  —  Instruction Set Architecture
   16-bit fixed-width instructions
   ============================================================

   Instruction Encoding (16 bits):
   ┌─────────────┬────────┬────────┬──────────────┐
    │  OPCODE     │  MODE  │  DST   │  SRC / IMM   │
   │  [15:10]    │ [9:8]  │ [7:6]  │   [5:0]      │
   │  6 bits     │ 2 bits │ 2 bits │   6 bits     │
   └─────────────┴────────┴────────┴──────────────┘

   For DIRECT addressing (full 16-bit address):
     Word 1: [ OPCODE ][ MODE=DIRECT ][ DST ][ 000000 ]
     Word 2: [ full 16-bit address ]
   ============================================================ */

/* ------------------------------------------------------------
   Addressing Modes (2 bits)
   ------------------------------------------------------------ */
typedef enum {
    MODE_REG      = 0x0,   /* 00 — operand is a register          */
    MODE_IMM      = 0x1,   /* 01 — operand is a 6-bit immediate   */
    MODE_DIRECT   = 0x2,   /* 10 — operand is a memory address    */
    MODE_INDIRECT = 0x3    /* 11 — operand is address in register */
} AddressMode;

/* ------------------------------------------------------------
   Registers (2 bits each — 4 general purpose)
   ------------------------------------------------------------ */
typedef enum {
    REG_R0 = 0x0,   /* Argument 1 / Return value          */
    REG_R1 = 0x1,   /* Argument 2 / Scratch               */
    REG_R2 = 0x2,   /* Argument 3 / Scratch               */
    REG_R3 = 0x3    /* Frame Pointer (callee-saved)        */
    /* SP and PC are special registers — not encoded here  */
} Register;

#define NUM_GP_REGS  4      /* R0–R3                              */

/* ------------------------------------------------------------
   Flag Register bits
   ------------------------------------------------------------ */
#define FLAG_Z  (1 << 0)   /* Zero     — result was zero          */
#define FLAG_N  (1 << 1)   /* Negative — result was negative      */
#define FLAG_C  (1 << 2)   /* Carry    — unsigned overflow        */
#define FLAG_V  (1 << 3)   /* Overflow — signed overflow          */

/* ------------------------------------------------------------
   Opcodes (6 bits — up to 64 instructions)
   ------------------------------------------------------------ */
typedef enum {
    /* System */
    OP_NOP   = 0x00,   /* No operation                           */
    OP_HALT  = 0x01,   /* Stop execution                         */

    /* Data Movement */
    OP_MOV   = 0x02,   /* Rd = Rs | Rd = imm                     */
    OP_LOAD  = 0x03,   /* Rd = Memory[addr | Rs]                 */
    OP_STORE = 0x04,   /* Memory[addr | Rd] = Rs                 */
    OP_PUSH  = 0x05,   /* Memory[--SP] = Rs                      */
    OP_POP   = 0x06,   /* Rd = Memory[SP++]                      */

    /* Arithmetic */
    OP_ADD   = 0x07,   /* Rd = Rd + Rs/imm                       */
    OP_SUB   = 0x08,   /* Rd = Rd - Rs/imm                       */
    OP_MUL   = 0x09,   /* Rd = Rd * Rs/imm  (for factorial)      */
    OP_INC   = 0x0A,   /* Rd = Rd + 1                            */
    OP_DEC   = 0x0B,   /* Rd = Rd - 1                            */

    /* Logic */
    OP_AND   = 0x0C,   /* Rd = Rd & Rs/imm                       */
    OP_OR    = 0x0D,   /* Rd = Rd | Rs/imm                       */
    OP_XOR   = 0x0E,   /* Rd = Rd ^ Rs/imm                       */
    OP_NOT   = 0x0F,   /* Rd = ~Rd                               */
    OP_SHL   = 0x10,   /* Rd = Rd << imm                         */
    OP_SHR   = 0x11,   /* Rd = Rd >> imm                         */

    /* Compare */
    OP_CMP   = 0x12,   /* flags = Rd - Rs/imm  (no result stored)*/

    /* Jumps */
    OP_JMP   = 0x13,   /* PC = addr  (unconditional)             */
    OP_JZ    = 0x14,   /* PC = addr  if Z flag set               */
    OP_JNZ   = 0x15,   /* PC = addr  if Z flag clear             */
    OP_JL    = 0x16,   /* PC = addr  if N flag set               */
    OP_JGE   = 0x17,   /* PC = addr  if N flag clear             */
    OP_JC    = 0x18,   /* PC = addr  if C flag set               */

    /* Subroutine */
    OP_CALL  = 0x19,   /* push PC; PC = addr                     */
    OP_RET   = 0x1A,   /* PC = pop()                             */

    /* I/O (Memory-Mapped) */
    OP_IN    = 0x1B,   /* Rd = MMIO[port]   (timer read)         */
    OP_OUT   = 0x1C,   /* MMIO[port] = Rs   (timer write)        */

    /* Extended data movement */
    OP_LOADB = 0x1D,   /* Rd = Memory[Rs]  — 8-bit byte load     */
    OP_MOVW  = 0x1E,   /* Rd = 16-bit immediate (extra word)      */

} Opcode;

#define NUM_OPCODES  0x1F   /* total opcodes defined              */

/* ------------------------------------------------------------
   Instruction encoding helpers
   ------------------------------------------------------------ */

/* Build a 16-bit instruction word */
#define MAKE_INSTR(op, mode, dst, src) \
    ((uint16_t)(((op) << 10) | ((mode) << 8) | ((dst) << 6) | ((src) & 0x3F)))

/* Extract fields from a 16-bit instruction */
#define INSTR_OPCODE(i)  (((i) >> 10) & 0x3F)
#define INSTR_MODE(i)    (((i) >>  8) & 0x03)
#define INSTR_DST(i)     (((i) >>  6) & 0x03)
#define INSTR_SRC(i)     ((i)         & 0x3F)

/* When MODE_IMM: sign-extend 6-bit immediate to int16_t */
#define SIGN_EXT6(v)  ((int16_t)(((v) & 0x20) ? ((v) | 0xFFC0) : (v)))

/* When MODE_REG: lower 2 bits of SRC field are the register */
#define INSTR_SRC_REG(i)  ((i) & 0x03)

/* ------------------------------------------------------------
   Memory Map  (64KB address space)
   ------------------------------------------------------------ */
#define MEM_SIZE          0x10000u   /* 64 KB total               */

#define DATA_SEG_BASE     0x0000u    /* Global variables, strings */
#define DATA_SEG_SIZE     0x1000u    /* 4 KB                      */
#define DATA_SEG_END      (DATA_SEG_BASE + DATA_SEG_SIZE - 1)

#define STACK_SEG_BASE    0x1000u    /* Stack grows downward      */
#define STACK_SEG_SIZE    0x1000u    /* 4 KB                      */
#define STACK_SEG_END     (STACK_SEG_BASE + STACK_SEG_SIZE - 1)
#define STACK_INIT_SP     STACK_SEG_END   /* SP starts at top     */

#define CODE_SEG_BASE     0x2000u    /* Program instructions      */
#define CODE_SEG_SIZE     0xD000u    /* 52 KB                     */
#define CODE_SEG_END      (CODE_SEG_BASE + CODE_SEG_SIZE - 1)

#define MMIO_BASE         0xF000u    /* Memory-mapped I/O         */
#define MMIO_SIZE         0x0100u    /* 256 bytes                 */
#define MMIO_END          (MMIO_BASE + MMIO_SIZE - 1)

/* MMIO Port addresses */
#define MMIO_TIMER_COUNT  0xF000u    /* Timer tick count (read)   */
#define MMIO_TIMER_CTRL   0xF001u    /* Timer control: 1=start    */
#define MMIO_STDOUT       0xF002u    /* Write char to stdout      */
#define MMIO_STDIN        0xF003u    /* Read char from stdin      */

/* ------------------------------------------------------------
   Calling Convention
   ------------------------------------------------------------
   R0  — Argument 1 / Return value
   R1  — Argument 2 / Scratch (caller-saved)
   R2  — Argument 3 / Scratch (caller-saved)
   R3  — Frame Pointer FP    (callee-saved)

   Stack frame layout (grows downward):
   ┌────────────────────────┐  ← old SP before CALL
   │  Return Address (PC)   │  pushed by CALL
   │  Saved R3 (old FP)     │  pushed by callee prologue
   │  Local variables       │
   └────────────────────────┘  ← current SP
   ------------------------------------------------------------ */

/* Instruction size in bytes (all instructions are 16-bit = 2 bytes)
   DIRECT mode instructions use an extra 2-byte word for the address */
#define INSTR_SIZE        2u
#define INSTR_SIZE_DIRECT 4u

#endif /* ISA_H */

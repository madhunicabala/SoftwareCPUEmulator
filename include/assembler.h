#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdint.h>
#include "isa.h"

/* ============================================================
   SoftCPU-C Assembler
   Converts .asm source files → .bin machine code

   Two-pass design:
     Pass 1 — scan all lines, collect label → address mappings
     Pass 2 — encode each instruction to 16-bit binary

   Assembly syntax:
     label:               ; defines a label at current address
     MNEMONIC             ; no operands   e.g. NOP, HALT, RET
     MNEMONIC Rd          ; one operand   e.g. PUSH R0, POP R1
     MNEMONIC Rd, Rs      ; reg, reg      e.g. MOV R0, R1
     MNEMONIC Rd, #imm    ; reg, imm      e.g. MOV R0, #5
     MNEMONIC Rd, [addr]  ; reg, direct   e.g. LOAD R0, [0x0010]
     MNEMONIC Rd, [Rs]    ; reg, indirect e.g. LOAD R0, [R1]
     MNEMONIC label       ; jump target   e.g. JMP loop
     ; comment            ; anything after ; is ignored
   ============================================================ */

/* ------------------------------------------------------------
   Limits
   ------------------------------------------------------------ */
#define MAX_LABELS       256    /* max labels per source file     */
#define MAX_LABEL_LEN    64     /* max characters in a label name */
#define MAX_LINE_LEN     256    /* max characters per source line */
#define MAX_TOKENS       8      /* max tokens per line            */
#define MAX_TOKEN_LEN    64     /* max characters per token       */
#define MAX_LINES        4096   /* max lines per source file      */

/* ------------------------------------------------------------
   Token types
   ------------------------------------------------------------ */
typedef enum {
    TOK_MNEMONIC,    /* instruction name: MOV, ADD, JMP ...      */
    TOK_REGISTER,    /* register name: R0, R1, R2, R3            */
    TOK_IMMEDIATE,   /* immediate value: #5, #0xFF, #-3          */
    TOK_LABEL_DEF,   /* label definition: loop:                  */
    TOK_LABEL_REF,   /* label reference used as jump target      */
    TOK_ADDR_DIRECT, /* direct memory address: [0x0010]          */
    TOK_ADDR_INDIR,  /* indirect register address: [R1]          */
    TOK_UNKNOWN
} TokenType;

/* ------------------------------------------------------------
   Token — one lexical unit from a source line
   ------------------------------------------------------------ */
typedef struct {
    TokenType type;
    char      text[MAX_TOKEN_LEN];  /* raw text of the token     */
    int16_t   int_val;              /* parsed value for IMM      */
    uint8_t   reg_val;              /* register index 0–3        */
    uint16_t  addr_val;             /* address for DIRECT mode   */
} Token;

/* ------------------------------------------------------------
   Symbol table entry — maps label name → address
   ------------------------------------------------------------ */
typedef struct {
    char     name[MAX_LABEL_LEN];
    uint16_t address;              /* absolute address in memory */
    int      defined;              /* 1 = defined, 0 = forward   */
} Symbol;

/* ------------------------------------------------------------
   Symbol table
   ------------------------------------------------------------ */
typedef struct {
    Symbol entries[MAX_LABELS];
    int    count;
} SymbolTable;

/* ------------------------------------------------------------
   Parsed line — result of lexing one source line
   ------------------------------------------------------------ */
typedef struct {
    int    line_num;              /* source line number (1-based) */
    Token  tokens[MAX_TOKENS];   /* tokens on this line          */
    int    token_count;
    int    has_label;            /* 1 if line starts with label: */
    char   label[MAX_LABEL_LEN]; /* label name if has_label      */

    /* Directive support */
    int    is_string_directive;       /* 1 if line is a .string directive */
    char   string_data[MAX_LINE_LEN]; /* escape-processed string content  */
} ParsedLine;

/* ------------------------------------------------------------
   Assembler context — state shared across both passes
   ------------------------------------------------------------ */
typedef struct {
    ParsedLine lines[MAX_LINES];
    int        line_count;

    SymbolTable symbols;

    uint8_t    output[CODE_SEG_SIZE];  /* encoded binary output  */
    uint32_t   output_size;            /* bytes written          */

    uint16_t   current_addr;           /* address counter        */
    int        errors;                 /* error count            */
} AsmContext;

/* ------------------------------------------------------------
   Mnemonic table entry — maps name string → opcode
   ------------------------------------------------------------ */
typedef struct {
    const char *name;
    Opcode      opcode;
    int         operand_count; /* 0, 1, or 2                     */
} MnemonicEntry;

/* ------------------------------------------------------------
   Lexer — public interface
   ------------------------------------------------------------ */

/* Initialise assembler context to zero state */
void asm_init(AsmContext *ctx);

/* Lex entire source file into ctx->lines[]
   Returns number of lines parsed, -1 on error */
int  lex_file(AsmContext *ctx, const char *source_path);

/* Lex a single line of source text into a ParsedLine
   Returns number of tokens found */
int  lex_line(const char *line, int line_num, ParsedLine *out);

/* ------------------------------------------------------------
   Assembler — public interface
   ------------------------------------------------------------ */

/* Pass 1: scan ctx->lines[], populate ctx->symbols
   Returns 0 on success, -1 on error */
int  asm_pass1(AsmContext *ctx);

/* Pass 2: encode all instructions into ctx->output[]
   Returns 0 on success, -1 on error */
int  asm_pass2(AsmContext *ctx);

/* Write ctx->output[] to a binary file
   Returns 0 on success, -1 on error */
int  asm_write_bin(AsmContext *ctx, const char *out_path);

/* Top-level: assemble source_path → out_path
   Returns 0 on success, -1 on error */
int  assemble(const char *source_path, const char *out_path);

/* ------------------------------------------------------------
   Symbol table helpers
   ------------------------------------------------------------ */
int      sym_add(SymbolTable *st, const char *name, uint16_t address);
int      sym_find(SymbolTable *st, const char *name);   /* returns index or -1 */
uint16_t sym_resolve(SymbolTable *st, const char *name, int *found);

/* ------------------------------------------------------------
   Utility
   ------------------------------------------------------------ */
int  parse_register(const char *text, uint8_t *out_reg);
int  parse_immediate(const char *text, int16_t *out_val);
int  parse_mnemonic(const char *text, Opcode *out_op, int *out_operands);
void asm_error(AsmContext *ctx, int line_num, const char *fmt, ...);

#endif /* ASSEMBLER_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "assembler.h"

/* ============================================================
   SoftCPU-C Lexer
   Reads .asm source text line by line and breaks each line
   into tokens that the assembler's two passes can process.
   ============================================================ */

/* ------------------------------------------------------------
   Mnemonic lookup table
   ------------------------------------------------------------ */
static const MnemonicEntry mnemonic_table[] = {
    { "NOP",   OP_NOP,   0 },
    { "HALT",  OP_HALT,  0 },
    { "MOV",   OP_MOV,   2 },
    { "LOAD",  OP_LOAD,  2 },
    { "STORE", OP_STORE, 2 },
    { "PUSH",  OP_PUSH,  1 },
    { "POP",   OP_POP,   1 },
    { "ADD",   OP_ADD,   2 },
    { "SUB",   OP_SUB,   2 },
    { "MUL",   OP_MUL,   2 },
    { "INC",   OP_INC,   1 },
    { "DEC",   OP_DEC,   1 },
    { "AND",   OP_AND,   2 },
    { "OR",    OP_OR,    2 },
    { "XOR",   OP_XOR,   2 },
    { "NOT",   OP_NOT,   1 },
    { "SHL",   OP_SHL,   2 },
    { "SHR",   OP_SHR,   2 },
    { "CMP",   OP_CMP,   2 },
    { "JMP",   OP_JMP,   1 },
    { "JZ",    OP_JZ,    1 },
    { "JNZ",   OP_JNZ,   1 },
    { "JL",    OP_JL,    1 },
    { "JGE",   OP_JGE,   1 },
    { "JC",    OP_JC,    1 },
    { "CALL",  OP_CALL,  1 },
    { "RET",   OP_RET,   0 },
    { "IN",    OP_IN,    2 },
    { "OUT",   OP_OUT,   2 },
    { NULL,    0,        0 }
};

/* ------------------------------------------------------------
   Error reporter
   ------------------------------------------------------------ */
void asm_error(AsmContext *ctx, int line_num, const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "[ASM] Error at line %d: ", line_num);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    if (ctx) ctx->errors++;
}

static void str_toupper(char *s) {
    for (; *s; s++) *s = (char)toupper((unsigned char)*s);
}

static char *str_trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

/* ------------------------------------------------------------
   parse_register — R0/R1/R2/R3
   ------------------------------------------------------------ */
int parse_register(const char *text, uint8_t *out_reg) {
    if (!text || strlen(text) != 2) return 0;
    if (toupper((unsigned char)text[0]) != 'R') return 0;
    char c = text[1];
    if (c < '0' || c > '3') return 0;
    *out_reg = (uint8_t)(c - '0');
    return 1;
}

/* ------------------------------------------------------------
   parse_immediate — #5, #0xFF, #-3, #0b1010
   ------------------------------------------------------------ */
int parse_immediate(const char *text, int16_t *out_val) {
    if (!text || text[0] != '#') return 0;
    const char *num = text + 1;
    char *end;
    long val;
    if (num[0] == '0' && (num[1] == 'x' || num[1] == 'X'))
        val = strtol(num, &end, 16);
    else if (num[0] == '0' && (num[1] == 'b' || num[1] == 'B'))
        val = strtol(num + 2, &end, 2);
    else
        val = strtol(num, &end, 10);
    if (end == num || *end != '\0') return 0;
    *out_val = (int16_t)val;
    return 1;
}

/* ------------------------------------------------------------
   parse_mnemonic
   ------------------------------------------------------------ */
int parse_mnemonic(const char *text, Opcode *out_op, int *out_operands) {
    char upper[MAX_TOKEN_LEN];
    strncpy(upper, text, MAX_TOKEN_LEN - 1);
    upper[MAX_TOKEN_LEN - 1] = '\0';
    str_toupper(upper);
    for (int i = 0; mnemonic_table[i].name != NULL; i++) {
        if (strcmp(upper, mnemonic_table[i].name) == 0) {
            if (out_op)       *out_op       = mnemonic_table[i].opcode;
            if (out_operands) *out_operands = mnemonic_table[i].operand_count;
            return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------
   lex_token — classify a single token string
   ------------------------------------------------------------ */
static Token lex_token(const char *raw) {
    Token tok;
    memset(&tok, 0, sizeof(tok));
    strncpy(tok.text, raw, MAX_TOKEN_LEN - 1);

    /* Register */
    if (parse_register(raw, &tok.reg_val)) {
        tok.type = TOK_REGISTER;
        return tok;
    }

    /* Immediate */
    if (raw[0] == '#') {
        tok.type = parse_immediate(raw, &tok.int_val) ? TOK_IMMEDIATE : TOK_UNKNOWN;
        return tok;
    }

    /* Bracket expression [...]  */
    if (raw[0] == '[' && raw[strlen(raw) - 1] == ']') {
        char inner[MAX_TOKEN_LEN];
        size_t inner_len = strlen(raw) - 2;
        strncpy(inner, raw + 1, inner_len);
        inner[inner_len] = '\0';
        str_trim(inner);

        uint8_t reg;
        if (parse_register(inner, &reg)) {
            tok.type    = TOK_ADDR_INDIR;
            tok.reg_val = reg;
            return tok;
        }
        char *end;
        long addr = (inner[0] == '0' && (inner[1] == 'x' || inner[1] == 'X'))
                    ? strtol(inner, &end, 16)
                    : strtol(inner, &end, 10);
        if (end != inner && *end == '\0') {
            tok.type     = TOK_ADDR_DIRECT;
            tok.addr_val = (uint16_t)addr;
        } else {
            tok.type = TOK_UNKNOWN;
        }
        return tok;
    }

    /* Mnemonic */
    Opcode op; int operands;
    if (parse_mnemonic(raw, &op, &operands)) {
        tok.type = TOK_MNEMONIC;
        return tok;
    }

    /* Label reference */
    int ok = 1;
    for (int i = 0; raw[i]; i++)
        if (!isalnum((unsigned char)raw[i]) && raw[i] != '_') { ok = 0; break; }
    if (ok && strlen(raw) > 0) {
        tok.type = TOK_LABEL_REF;
        return tok;
    }

    tok.type = TOK_UNKNOWN;
    return tok;
}

/* ------------------------------------------------------------
   lex_line
   ------------------------------------------------------------ */
int lex_line(const char *line, int line_num, ParsedLine *out) {
    memset(out, 0, sizeof(ParsedLine));
    out->line_num = line_num;

    char buf[MAX_LINE_LEN];
    strncpy(buf, line, MAX_LINE_LEN - 1);
    buf[MAX_LINE_LEN - 1] = '\0';

    char *comment = strchr(buf, ';');
    if (comment) *comment = '\0';

    char *s = str_trim(buf);
    if (strlen(s) == 0) return 0;

    /* Detect label definition */
    char *colon = strchr(s, ':');
    if (colon) {
        int label_end = (int)(colon - s);
        int valid = 1;
        for (int i = 0; i < label_end; i++)
            if (isspace((unsigned char)s[i])) { valid = 0; break; }
        if (valid && label_end > 0) {
            strncpy(out->label, s, label_end);
            out->label[label_end] = '\0';
            str_toupper(out->label);
            out->has_label = 1;
            s = str_trim(colon + 1);
            if (strlen(s) == 0) return 0;
        }
    }

    /* Tokenise */
    char *p = s;
    int   tok_count = 0;

    while (*p && tok_count < MAX_TOKENS) {
        while (*p == ' ' || *p == '\t' || *p == ',') p++;
        if (!*p) break;

        char word[MAX_TOKEN_LEN];
        int  wi = 0;

        if (*p == '[') {
            while (*p && *p != ']' && wi < MAX_TOKEN_LEN - 1) word[wi++] = *p++;
            if (*p == ']') word[wi++] = *p++;
        } else {
            while (*p && *p != ' ' && *p != '\t' && *p != ',' && wi < MAX_TOKEN_LEN - 1)
                word[wi++] = *p++;
        }
        word[wi] = '\0';
        if (wi == 0) break;

        /* Uppercase non-special tokens */
        if (word[0] != '#' && word[0] != '[')
            str_toupper(word);

        out->tokens[tok_count++] = lex_token(word);
    }

    out->token_count = tok_count;
    return tok_count;
}

/* ------------------------------------------------------------
   lex_file
   ------------------------------------------------------------ */
int lex_file(AsmContext *ctx, const char *source_path) {
    FILE *f = fopen(source_path, "r");
    if (!f) {
        fprintf(stderr, "[ASM] Cannot open: %s\n", source_path);
        return -1;
    }

    char line[MAX_LINE_LEN];
    int  line_num = 0, parsed = 0;

    while (fgets(line, sizeof(line), f) && parsed < MAX_LINES) {
        line_num++;
        char *nl = strchr(line, '\n'); if (nl) *nl = '\0';
        char *cr = strchr(line, '\r'); if (cr) *cr = '\0';

        ParsedLine pl;
        int toks = lex_line(line, line_num, &pl);
        if (pl.has_label || toks > 0)
            ctx->lines[parsed++] = pl;
    }

    fclose(f);
    ctx->line_count = parsed;
    printf("[ASM] Lexed %d lines from %s\n", parsed, source_path);
    return parsed;
}

/* ------------------------------------------------------------
   asm_init
   ------------------------------------------------------------ */
void asm_init(AsmContext *ctx) {
    memset(ctx, 0, sizeof(AsmContext));
    ctx->current_addr = CODE_SEG_BASE;
}

/* ------------------------------------------------------------
   Symbol table
   ------------------------------------------------------------ */
int sym_add(SymbolTable *st, const char *name, uint16_t address) {
    if (st->count >= MAX_LABELS) {
        fprintf(stderr, "[ASM] Symbol table full\n");
        return -1;
    }
    for (int i = 0; i < st->count; i++) {
        if (strcmp(st->entries[i].name, name) == 0) {
            fprintf(stderr, "[ASM] Duplicate label: %s\n", name);
            return -1;
        }
    }
    strncpy(st->entries[st->count].name, name, MAX_LABEL_LEN - 1);
    st->entries[st->count].address = address;
    st->entries[st->count].defined = 1;
    st->count++;
    return 0;
}

int sym_find(SymbolTable *st, const char *name) {
    for (int i = 0; i < st->count; i++)
        if (strcmp(st->entries[i].name, name) == 0) return i;
    return -1;
}

uint16_t sym_resolve(SymbolTable *st, const char *name, int *found) {
    int idx = sym_find(st, name);
    if (idx < 0) { if (found) *found = 0; return 0; }
    if (found) *found = 1;
    return st->entries[idx].address;
}

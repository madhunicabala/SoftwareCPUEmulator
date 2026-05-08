#include <stdio.h>
#include <string.h>
#include "assembler.h"

/*
 * Links against lexer.o only (assembler.c has its own main).
 * Tests: parse_register, parse_immediate, parse_mnemonic,
 *        lex_line, symbol table, asm_init.
 */

static int tests_run = 0, tests_passed = 0;

#define ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { tests_passed++; printf("  PASS  %s\n", msg); } \
    else       { printf("  FAIL  %s  (%s:%d)\n", msg, __FILE__, __LINE__); } \
} while(0)

/* ── parse_register ────────────────────────────────────────── */

static void test_parse_register(void) {
    uint8_t reg;

    ASSERT(parse_register("R0", &reg) && reg == 0, "parse_register: R0 = 0");
    ASSERT(parse_register("R1", &reg) && reg == 1, "parse_register: R1 = 1");
    ASSERT(parse_register("R2", &reg) && reg == 2, "parse_register: R2 = 2");
    ASSERT(parse_register("R3", &reg) && reg == 3, "parse_register: R3 = 3");

    ASSERT(!parse_register("R4",  &reg), "parse_register: R4 invalid");
    ASSERT(!parse_register("R9",  &reg), "parse_register: R9 invalid");
    ASSERT(!parse_register("X1",  &reg), "parse_register: non-R prefix invalid");
    ASSERT(!parse_register("R",   &reg), "parse_register: lone R invalid");
    /* parse_register uses toupper internally — lowercase 'r' is accepted */
    ASSERT(parse_register("r0",  &reg) && reg == 0, "parse_register: r0 accepted (toupper)");
    ASSERT(!parse_register("R00", &reg), "parse_register: too long invalid");
    ASSERT(!parse_register("",    &reg), "parse_register: empty invalid");
    ASSERT(!parse_register(NULL,  &reg), "parse_register: NULL invalid");
}

/* ── parse_immediate ──────────────────────────────────────── */

static void test_parse_immediate(void) {
    int16_t val;

    ASSERT(parse_immediate("#0",    &val) && val ==  0,   "parse_immediate: #0 = 0");
    ASSERT(parse_immediate("#5",    &val) && val ==  5,   "parse_immediate: #5 = 5");
    ASSERT(parse_immediate("#31",   &val) && val == 31,   "parse_immediate: #31 = 31");
    ASSERT(parse_immediate("#-1",   &val) && val == -1,   "parse_immediate: #-1 = -1");
    ASSERT(parse_immediate("#-32",  &val) && val == -32,  "parse_immediate: #-32 = -32");
    ASSERT(parse_immediate("#0xFF", &val) && val == 0xFF, "parse_immediate: #0xFF = 255");
    ASSERT(parse_immediate("#0XAB", &val) && val == 0xAB, "parse_immediate: #0XAB (upper X)");
    ASSERT(parse_immediate("#0b1010",&val)&& val == 10,   "parse_immediate: #0b1010 = 10");
    ASSERT(parse_immediate("#0B11", &val) && val ==  3,   "parse_immediate: #0B11 = 3");

    ASSERT(!parse_immediate("5",    &val), "parse_immediate: no # prefix invalid");
    ASSERT(!parse_immediate("#",    &val), "parse_immediate: bare # invalid");
    ASSERT(!parse_immediate("#xyz", &val), "parse_immediate: non-numeric invalid");
    ASSERT(!parse_immediate(NULL,   &val), "parse_immediate: NULL invalid");
}

/* ── parse_mnemonic ──────────────────────────────────────── */

static void test_parse_mnemonic(void) {
    Opcode op; int operands;

    ASSERT(parse_mnemonic("NOP",  &op, &operands) && op==OP_NOP  && operands==0, "mnemonic: NOP");
    ASSERT(parse_mnemonic("HALT", &op, &operands) && op==OP_HALT && operands==0, "mnemonic: HALT");
    ASSERT(parse_mnemonic("MOV",  &op, &operands) && op==OP_MOV  && operands==2, "mnemonic: MOV");
    ASSERT(parse_mnemonic("LOAD", &op, &operands) && op==OP_LOAD && operands==2, "mnemonic: LOAD");
    ASSERT(parse_mnemonic("ADD",  &op, &operands) && op==OP_ADD  && operands==2, "mnemonic: ADD");
    ASSERT(parse_mnemonic("SUB",  &op, &operands) && op==OP_SUB  && operands==2, "mnemonic: SUB");
    ASSERT(parse_mnemonic("JMP",  &op, &operands) && op==OP_JMP  && operands==1, "mnemonic: JMP");
    ASSERT(parse_mnemonic("JZ",   &op, &operands) && op==OP_JZ   && operands==1, "mnemonic: JZ");
    ASSERT(parse_mnemonic("CALL", &op, &operands) && op==OP_CALL && operands==1, "mnemonic: CALL");
    ASSERT(parse_mnemonic("RET",  &op, &operands) && op==OP_RET  && operands==0, "mnemonic: RET");
    ASSERT(parse_mnemonic("PUSH", &op, &operands) && op==OP_PUSH && operands==1, "mnemonic: PUSH");
    ASSERT(parse_mnemonic("POP",  &op, &operands) && op==OP_POP  && operands==1, "mnemonic: POP");
    ASSERT(parse_mnemonic("IN",   &op, &operands) && op==OP_IN   && operands==2, "mnemonic: IN");
    ASSERT(parse_mnemonic("OUT",  &op, &operands) && op==OP_OUT  && operands==2, "mnemonic: OUT");
    ASSERT(parse_mnemonic("MOVW", &op, &operands) && op==OP_MOVW && operands==2, "mnemonic: MOVW");
    ASSERT(parse_mnemonic("LOADB",&op, &operands) && op==OP_LOADB&& operands==2, "mnemonic: LOADB");

    /* Case-insensitive */
    ASSERT(parse_mnemonic("mov",  &op, &operands) && op==OP_MOV, "mnemonic: lowercase accepted");
    ASSERT(parse_mnemonic("Add",  &op, &operands) && op==OP_ADD, "mnemonic: mixed case accepted");

    /* Unknown */
    ASSERT(!parse_mnemonic("NOTANOP", &op, &operands), "mnemonic: unknown returns 0");
    ASSERT(!parse_mnemonic("",        &op, &operands), "mnemonic: empty returns 0");
}

/* ── lex_line ─────────────────────────────────────────────── */

static void test_lex_basic(void) {
    ParsedLine pl;
    int n;

    /* MOV R0, #5 → 3 tokens */
    n = lex_line("  MOV R0, #5", 1, &pl);
    ASSERT(n == 3, "lex MOV R0,#5: 3 tokens");
    ASSERT(pl.tokens[0].type == TOK_MNEMONIC,  "lex: tok[0] mnemonic");
    ASSERT(pl.tokens[1].type == TOK_REGISTER && pl.tokens[1].reg_val == 0,
           "lex: tok[1] R0");
    ASSERT(pl.tokens[2].type == TOK_IMMEDIATE && pl.tokens[2].int_val == 5,
           "lex: tok[2] #5");

    /* NOP → 1 token */
    n = lex_line("NOP", 2, &pl);
    ASSERT(n == 1 && pl.tokens[0].type == TOK_MNEMONIC, "lex NOP: 1 mnemonic token");

    /* ADD R1, R2 */
    n = lex_line("ADD R1, R2", 3, &pl);
    ASSERT(n == 3, "lex ADD R1,R2: 3 tokens");
    ASSERT(pl.tokens[1].reg_val == 1, "lex ADD: dst = R1");
    ASSERT(pl.tokens[2].reg_val == 2, "lex ADD: src = R2");
}

static void test_lex_empty(void) {
    ParsedLine pl;

    ASSERT(lex_line("", 1, &pl)                    == 0, "lex: empty line → 0 tokens");
    ASSERT(lex_line("   ", 2, &pl)                 == 0, "lex: whitespace only → 0");
    ASSERT(lex_line("; comment", 3, &pl)            == 0, "lex: comment only → 0");
    ASSERT(lex_line("  ; indented comment", 4, &pl) == 0, "lex: indented comment → 0");
    ASSERT(lex_line("HALT ; end", 5, &pl)           == 1, "lex: instr + comment → 1 token");
}

static void test_lex_label(void) {
    ParsedLine pl;
    int n;

    /* Label only */
    n = lex_line("loop:", 1, &pl);
    ASSERT(pl.has_label,                        "lex label: has_label set");
    ASSERT(strcmp(pl.label, "LOOP") == 0,       "lex label: uppercase 'LOOP'");
    ASSERT(n == 0,                              "lex label only: 0 instr tokens");

    /* Label + instruction */
    n = lex_line("done: HALT", 2, &pl);
    ASSERT(pl.has_label,                        "lex label+instr: has_label");
    ASSERT(strcmp(pl.label, "DONE") == 0,       "lex label+instr: label text");
    ASSERT(n == 1,                              "lex label+instr: 1 instr token");
    ASSERT(pl.tokens[0].type == TOK_MNEMONIC,  "lex label+instr: mnemonic token");

    /* Label + 2-operand */
    n = lex_line("start: MOV R0, #0", 3, &pl);
    ASSERT(pl.has_label,                  "lex label+mov: has_label");
    ASSERT(strcmp(pl.label, "START") == 0,"lex label+mov: label = START");
    ASSERT(n == 3,                        "lex label+mov: 3 tokens");
}

static void test_lex_addressing(void) {
    ParsedLine pl;
    int n;

    /* Direct address [0x0010] */
    n = lex_line("LOAD R1, [0x0010]", 1, &pl);
    ASSERT(n == 3,                                       "lex direct addr: 3 tokens");
    ASSERT(pl.tokens[2].type == TOK_ADDR_DIRECT,         "lex direct addr: TOK_ADDR_DIRECT");
    ASSERT(pl.tokens[2].addr_val == 0x0010,              "lex direct addr: 0x0010");

    /* Indirect register [R2] */
    n = lex_line("LOAD R1, [R2]", 2, &pl);
    ASSERT(n == 3,                                       "lex indirect: 3 tokens");
    ASSERT(pl.tokens[2].type == TOK_ADDR_INDIR,          "lex indirect: TOK_ADDR_INDIR");
    ASSERT(pl.tokens[2].reg_val == 2,                    "lex indirect: reg = R2");

    /* Negative immediate */
    n = lex_line("MOV R0, #-10", 3, &pl);
    ASSERT(n == 3,                                       "lex neg imm: 3 tokens");
    ASSERT(pl.tokens[2].type == TOK_IMMEDIATE,            "lex neg imm: TOK_IMMEDIATE");
    ASSERT(pl.tokens[2].int_val == -10,                  "lex neg imm: -10");

    /* Label reference (jump target) */
    n = lex_line("JMP myloop", 4, &pl);
    ASSERT(n == 2,                                       "lex label ref: 2 tokens");
    ASSERT(pl.tokens[1].type == TOK_LABEL_REF,           "lex label ref: TOK_LABEL_REF");
}

static void test_lex_string_directive(void) {
    ParsedLine pl;
    int n;

    n = lex_line("msg: .string \"Hello\"", 1, &pl);
    ASSERT(pl.has_label,                        "string dir: has_label");
    ASSERT(pl.is_string_directive,              "string dir: is_string_directive");
    ASSERT(strcmp(pl.string_data, "Hello") == 0,"string dir: data = 'Hello'");
    ASSERT(n == 0,                              "string dir: 0 instr tokens");

    n = lex_line(".string \"AB\"", 2, &pl);
    ASSERT(pl.is_string_directive,              "string dir (no label): is_string_directive");
    ASSERT(strcmp(pl.string_data, "AB") == 0,   "string dir: data = 'AB'");
}

/* ── Symbol table ─────────────────────────────────────────── */

static void test_symbol_table(void) {
    SymbolTable st;
    memset(&st, 0, sizeof(st));

    ASSERT(sym_add(&st, "START", 0x2000) == 0, "sym_add: first entry");
    ASSERT(sym_add(&st, "LOOP",  0x2010) == 0, "sym_add: second entry");
    ASSERT(st.count == 2,                       "sym_add: count = 2");

    /* Duplicate rejected */
    ASSERT(sym_add(&st, "START", 0x2020) != 0, "sym_add: duplicate rejected");
    ASSERT(st.count == 2,                       "sym_add: count unchanged after dup");

    /* sym_find */
    ASSERT(sym_find(&st, "START")  ==  0, "sym_find: START at index 0");
    ASSERT(sym_find(&st, "LOOP")   ==  1, "sym_find: LOOP at index 1");
    ASSERT(sym_find(&st, "DONE")   == -1, "sym_find: missing returns -1");

    /* sym_resolve */
    int found;
    uint16_t addr;

    addr = sym_resolve(&st, "LOOP", &found);
    ASSERT(found == 1 && addr == 0x2010, "sym_resolve: LOOP = 0x2010");

    addr = sym_resolve(&st, "MISSING", &found);
    ASSERT(found == 0, "sym_resolve: missing sets found=0");
    (void)addr;
}

/* ── asm_init ─────────────────────────────────────────────── */

static void test_asm_init(void) {
    AsmContext ctx;
    asm_init(&ctx);

    ASSERT(ctx.current_addr  == CODE_SEG_BASE, "asm_init: current_addr = CODE_SEG_BASE");
    ASSERT(ctx.errors        == 0,             "asm_init: errors = 0");
    ASSERT(ctx.line_count    == 0,             "asm_init: line_count = 0");
    ASSERT(ctx.output_size   == 0,             "asm_init: output_size = 0");
    ASSERT(ctx.symbols.count == 0,             "asm_init: symbol table empty");
}

int main(void) {
    printf("=== test_assembler ===\n");
    test_parse_register();
    test_parse_immediate();
    test_parse_mnemonic();
    test_lex_basic();
    test_lex_empty();
    test_lex_label();
    test_lex_addressing();
    test_lex_string_directive();
    test_symbol_table();
    test_asm_init();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}

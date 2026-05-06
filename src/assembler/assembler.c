#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "assembler.h"

/* ============================================================
   SoftCPU-C Assembler  —  Two-Pass Encoder
   ============================================================

   PASS 1:  Scan all ParsedLines.
            Every time a label definition is seen, record:
              label_name → current_address
            Advance current_address by instruction size
            (2 bytes normally, 4 bytes for DIRECT mode)
            so labels resolve to the right addresses.

   PASS 2:  Walk ParsedLines again.
            For each instruction, call encode_instruction()
            which looks up operand types, picks the addressing
            mode, builds the 16-bit word using MAKE_INSTR,
            and appends it (plus any address word) to output[].

   Output:  Raw binary written to .bin file starting at
            offset 0 — loaded by cpu_load() into CODE_SEG_BASE.
   ============================================================ */

/* ------------------------------------------------------------
   Forward declarations
   ------------------------------------------------------------ */
static int  encode_instruction(AsmContext *ctx, ParsedLine *pl);
static int  instruction_size(ParsedLine *pl);

/* ------------------------------------------------------------
   emit_word — append a 16-bit word to output buffer
               little-endian (low byte first)
   ------------------------------------------------------------ */
static void emit_word(AsmContext *ctx, uint16_t word) {
    if (ctx->output_size + 2 > CODE_SEG_SIZE) {
        fprintf(stderr, "[ASM] Output exceeds code segment size\n");
        ctx->errors++;
        return;
    }
    ctx->output[ctx->output_size++] = (uint8_t)(word & 0xFF);
    ctx->output[ctx->output_size++] = (uint8_t)((word >> 8) & 0xFF);
}

/* ------------------------------------------------------------
   instruction_size
   Returns how many bytes this instruction will occupy.
   Used in pass 1 to advance the address counter correctly.

   Rules:
   - No-operand instructions (NOP HALT RET):         2 bytes
   - One-operand register (PUSH POP INC DEC NOT):    2 bytes
   - Two-operand register-register (ADD R0, R1):     2 bytes
   - Two-operand register-immediate (MOV R0, #5):    2 bytes
   - Jump/Call with label or direct address:         4 bytes
     (instruction word + 16-bit address word)
   - LOAD/STORE with direct address:                 4 bytes
   - IN/OUT with port address:                       4 bytes
   ------------------------------------------------------------ */
static int instruction_size(ParsedLine *pl) {
    if (pl->token_count == 0) return 0;
    if (pl->tokens[0].type != TOK_MNEMONIC) return 0;

    Opcode op; int operands;
    if (!parse_mnemonic(pl->tokens[0].text, &op, &operands)) return 0;

    /* Jumps, CALL always need a 16-bit address word */
    switch (op) {
        case OP_JMP: case OP_JZ:  case OP_JNZ:
        case OP_JL:  case OP_JGE: case OP_JC:
        case OP_CALL:
            return 4;  /* instr word + address word */

        case OP_IN:  case OP_OUT:
            return 4;  /* instr word + port word     */

        case OP_LOAD: case OP_STORE:
            /* 4 bytes only if second operand is direct address */
            if (pl->token_count >= 2) {
                /* find non-mnemonic, non-dst token */
                for (int i = 1; i < pl->token_count; i++) {
                    if (pl->tokens[i].type == TOK_ADDR_DIRECT) return 4;
                }
            }
            return 2;

        default:
            return 2;
    }
}

/* ------------------------------------------------------------
   PASS 1 — collect all label → address mappings
   ------------------------------------------------------------ */
int asm_pass1(AsmContext *ctx) {
    ctx->current_addr = CODE_SEG_BASE;
    ctx->symbols.count = 0;

    for (int i = 0; i < ctx->line_count; i++) {
        ParsedLine *pl = &ctx->lines[i];

        /* Record label at current address */
        if (pl->has_label) {
            if (sym_add(&ctx->symbols, pl->label, ctx->current_addr) != 0) {
                ctx->errors++;
            } else {
                printf("[ASM] Pass1: label '%s' = 0x%04X\n",
                       pl->label, ctx->current_addr);
            }
        }

        /* Advance address by this instruction's size */
        if (pl->token_count > 0 && pl->tokens[0].type == TOK_MNEMONIC) {
            int size = instruction_size(pl);
            ctx->current_addr += (uint16_t)size;
        }
    }

    printf("[ASM] Pass 1 complete: %d labels, %d errors\n",
           ctx->symbols.count, ctx->errors);
    return (ctx->errors > 0) ? -1 : 0;
}

/* ------------------------------------------------------------
   encode_instruction — encode one ParsedLine into binary
   Called by pass 2 for every line with a mnemonic.
   ------------------------------------------------------------ */
static int encode_instruction(AsmContext *ctx, ParsedLine *pl) {
    if (pl->token_count == 0) return 0;
    if (pl->tokens[0].type != TOK_MNEMONIC) return 0;

    Opcode op; int operands;
    if (!parse_mnemonic(pl->tokens[0].text, &op, &operands)) {
        asm_error(ctx, pl->line_num, "Unknown mnemonic: %s", pl->tokens[0].text);
        return -1;
    }

    uint8_t  mode = MODE_REG;
    uint8_t  dst  = 0;
    uint8_t  src  = 0;
    uint16_t addr_word = 0;
    int      emit_addr = 0;

    /* ── No-operand instructions ─────────────────────────── */
    if (operands == 0) {
        emit_word(ctx, MAKE_INSTR(op, MODE_REG, 0, 0));
        return 0;
    }

    /* ── Single-operand instructions ─────────────────────── */
    if (operands == 1) {
        Token *t = &pl->tokens[1];

        switch (op) {
            /* PUSH Rd / POP Rd / INC Rd / DEC Rd / NOT Rd */
            case OP_PUSH: case OP_POP:
            case OP_INC:  case OP_DEC: case OP_NOT:
                if (t->type != TOK_REGISTER) {
                    asm_error(ctx, pl->line_num, "%s requires a register",
                              pl->tokens[0].text);
                    return -1;
                }
                emit_word(ctx, MAKE_INSTR(op, MODE_REG, t->reg_val, 0));
                return 0;

            /* Jumps: JMP / JZ / JNZ / JL / JGE / JC / CALL */
            case OP_JMP: case OP_JZ:  case OP_JNZ:
            case OP_JL:  case OP_JGE: case OP_JC:
            case OP_CALL:
                if (t->type == TOK_LABEL_REF) {
                    int found;
                    addr_word = sym_resolve(&ctx->symbols, t->text, &found);
                    if (!found) {
                        asm_error(ctx, pl->line_num,
                                  "Undefined label: %s", t->text);
                        return -1;
                    }
                } else if (t->type == TOK_ADDR_DIRECT) {
                    addr_word = t->addr_val;
                } else {
                    asm_error(ctx, pl->line_num,
                              "%s requires a label or address",
                              pl->tokens[0].text);
                    return -1;
                }
                emit_word(ctx, MAKE_INSTR(op, MODE_DIRECT, 0, 0));
                emit_word(ctx, addr_word);
                return 0;

            default:
                asm_error(ctx, pl->line_num,
                          "Unexpected single-operand op: %s",
                          pl->tokens[0].text);
                return -1;
        }
    }

    /* ── Two-operand instructions ─────────────────────────── */
    /* tokens[1] = dst operand,  tokens[2] = src operand     */
    if (pl->token_count < 3) {
        asm_error(ctx, pl->line_num, "%s requires two operands",
                  pl->tokens[0].text);
        return -1;
    }

    Token *tdst = &pl->tokens[1];
    Token *tsrc = &pl->tokens[2];

    /* DST must always be a register for two-operand ops
       (except STORE where dst encodes the source register) */
    if (op != OP_STORE && tdst->type != TOK_REGISTER) {
        asm_error(ctx, pl->line_num,
                  "%s: first operand must be a register",
                  pl->tokens[0].text);
        return -1;
    }
    if (op == OP_STORE && tdst->type != TOK_REGISTER) {
        asm_error(ctx, pl->line_num,
                  "STORE: first operand must be a register");
        return -1;
    }

    dst = tdst->reg_val;

    /* ── STORE Rd, [addr] / STORE Rd, [Rs] ── */
    if (op == OP_STORE) {
        if (tsrc->type == TOK_ADDR_DIRECT) {
            emit_word(ctx, MAKE_INSTR(op, MODE_DIRECT, dst, 0));
            emit_word(ctx, tsrc->addr_val);
        } else if (tsrc->type == TOK_ADDR_INDIR) {
            emit_word(ctx, MAKE_INSTR(op, MODE_INDIRECT, dst, tsrc->reg_val));
        } else {
            asm_error(ctx, pl->line_num,
                      "STORE: second operand must be [addr] or [Rs]");
            return -1;
        }
        return 0;
    }

    /* ── LOAD Rd, [addr] / LOAD Rd, [Rs] / LOAD Rd, Rs ── */
    if (op == OP_LOAD) {
        if (tsrc->type == TOK_ADDR_DIRECT) {
            emit_word(ctx, MAKE_INSTR(op, MODE_DIRECT, dst, 0));
            emit_word(ctx, tsrc->addr_val);
        } else if (tsrc->type == TOK_ADDR_INDIR) {
            emit_word(ctx, MAKE_INSTR(op, MODE_INDIRECT, dst, tsrc->reg_val));
        } else if (tsrc->type == TOK_REGISTER) {
            emit_word(ctx, MAKE_INSTR(op, MODE_REG, dst, tsrc->reg_val));
        } else {
            asm_error(ctx, pl->line_num,
                      "LOAD: invalid source operand");
            return -1;
        }
        return 0;
    }

    /* ── IN Rd, port / OUT Rd, port ── */
    if (op == OP_IN || op == OP_OUT) {
        if (tsrc->type == TOK_ADDR_DIRECT) {
            emit_word(ctx, MAKE_INSTR(op, MODE_DIRECT, dst, 0));
            emit_word(ctx, tsrc->addr_val);
        } else if (tsrc->type == TOK_IMMEDIATE) {
            /* port as immediate treated as direct address */
            emit_word(ctx, MAKE_INSTR(op, MODE_DIRECT, dst, 0));
            emit_word(ctx, (uint16_t)tsrc->int_val);
        } else {
            asm_error(ctx, pl->line_num,
                      "IN/OUT: port must be an address [0xF000] or #imm");
            return -1;
        }
        return 0;
    }

    /* ── General two-operand: ADD SUB MUL AND OR XOR CMP MOV SHL SHR ── */
    switch (tsrc->type) {
        case TOK_REGISTER:
            mode = MODE_REG;
            src  = tsrc->reg_val;
            break;

        case TOK_IMMEDIATE: {
            int16_t imm = tsrc->int_val;
            if (imm < -32 || imm > 31) {
                asm_error(ctx, pl->line_num,
                          "Immediate %d out of 6-bit signed range [-32, 31]",
                          imm);
                return -1;
            }
            mode = MODE_IMM;
            src  = (uint8_t)(imm & 0x3F);
            break;
        }

        case TOK_ADDR_DIRECT:
            mode      = MODE_DIRECT;
            src       = 0;
            addr_word = tsrc->addr_val;
            emit_addr = 1;
            break;

        case TOK_ADDR_INDIR:
            mode = MODE_INDIRECT;
            src  = tsrc->reg_val;
            break;

        default:
            asm_error(ctx, pl->line_num,
                      "Invalid source operand for %s",
                      pl->tokens[0].text);
            return -1;
    }

    emit_word(ctx, MAKE_INSTR(op, mode, dst, src));
    if (emit_addr) emit_word(ctx, addr_word);
    return 0;
}

/* ------------------------------------------------------------
   PASS 2 — encode all instructions
   ------------------------------------------------------------ */
int asm_pass2(AsmContext *ctx) {
    ctx->output_size  = 0;
    ctx->current_addr = CODE_SEG_BASE;

    for (int i = 0; i < ctx->line_count; i++) {
        ParsedLine *pl = &ctx->lines[i];

        if (pl->token_count > 0 && pl->tokens[0].type == TOK_MNEMONIC) {
            uint32_t before = ctx->output_size;
            if (encode_instruction(ctx, pl) != 0 && ctx->errors > 0)
                return -1;
            uint32_t written = ctx->output_size - before;
            ctx->current_addr += (uint16_t)written;
        }
    }

    printf("[ASM] Pass 2 complete: %u bytes encoded, %d errors\n",
           ctx->output_size, ctx->errors);
    return (ctx->errors > 0) ? -1 : 0;
}

/* ------------------------------------------------------------
   asm_write_bin — write output buffer to .bin file
   ------------------------------------------------------------ */
int asm_write_bin(AsmContext *ctx, const char *out_path) {
    FILE *f = fopen(out_path, "wb");
    if (!f) {
        fprintf(stderr, "[ASM] Cannot write: %s\n", out_path);
        return -1;
    }
    size_t written = fwrite(ctx->output, 1, ctx->output_size, f);
    fclose(f);
    if (written != ctx->output_size) {
        fprintf(stderr, "[ASM] Write error: %s\n", out_path);
        return -1;
    }
    printf("[ASM] Wrote %u bytes to %s\n", ctx->output_size, out_path);
    return 0;
}

/* ------------------------------------------------------------
   assemble — top-level entry point
   ------------------------------------------------------------ */
int assemble(const char *source_path, const char *out_path) {
    AsmContext ctx;
    asm_init(&ctx);

    printf("[ASM] Assembling %s → %s\n", source_path, out_path);

    if (lex_file(&ctx, source_path) < 0)  return -1;
    if (asm_pass1(&ctx) != 0)             return -1;
    if (asm_pass2(&ctx) != 0)             return -1;
    if (asm_write_bin(&ctx, out_path) != 0) return -1;

    printf("[ASM] Done. %u bytes, %d labels.\n",
           ctx.output_size, ctx.symbols.count);
    return 0;
}

/* ------------------------------------------------------------
   main — standalone assembler binary
   ------------------------------------------------------------ */
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <source.asm> <output.bin>\n", argv[0]);
        return 1;
    }
    return assemble(argv[1], argv[2]) == 0 ? 0 : 1;
}

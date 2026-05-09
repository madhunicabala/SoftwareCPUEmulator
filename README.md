# Emu16

A 16-bit software CPU emulator written in C. Built from scratch — custom ISA, ALU, emulator, and assembler — to simulate how a real processor fetches, decodes, and executes instructions.

Demonstrates the full fetch–decode–execute cycle, memory segmentation, a flag register, memory-mapped I/O, and a calling convention with stack frames. Runs four programs: Fibonacci, Factorial (recursive), Timer, and String output.

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Project Structure](#project-structure)
- [Build Requirements](#build-requirements)
- [Quick Start](#quick-start)
- [Usage](#usage)
- [Trace Viewer](#trace-viewer)
- [Instruction Set](#instruction-set)
- [Memory Map](#memory-map)
- [Calling Convention](#calling-convention)
- [Assembler](#assembler)
- [Example Programs](#example-programs)
- [Demo Video - FIBONACCI)](#demo-video)
- [Team](#team)

---

## Architecture Overview

### CPU Components

| Component | Description |
|-----------|-------------|
| **ALU** | Arithmetic and logic unit — handles ADD, SUB, MUL, AND, OR, XOR, shifts, and compare |
| **Registers** | 4 general-purpose (R0–R3) + PC, SP — all 16-bit |
| **Control Unit** | Decodes the 16-bit instruction word and drives the ALU and memory |
| **Memory** | Flat 64KB address space split into Data, Stack, Code, and MMIO segments |
| **Flag Register** | 4 bits: Zero (Z), Negative (N), Carry (C), Overflow (V) |
| **MMIO** | Memory-mapped I/O for timer control and character I/O |

### Instruction Encoding

Every instruction is exactly **16 bits wide** — fixed-width, no variable-length encoding.

```
 15      10  9    8  7    6  5          0
┌──────────┬──────┬──────┬──────────────┐
│  OPCODE  │ MODE │  DST │  SRC / IMM   │
│  6 bits  │2 bits│2 bits│   6 bits     │
└──────────┴──────┴──────┴──────────────┘
```

For `DIRECT` mode instructions that need a full 16-bit address, a second 16-bit word follows the instruction word immediately in memory.

### Addressing Modes

| Code | Name | Description |
|------|------|-------------|
| `00` | REG | Operand is a register (R0–R3) |
| `01` | IMM | 6-bit sign-extended immediate (−32 to +31) |
| `10` | DIRECT | Next word in memory is the full 16-bit address |
| `11` | INDIRECT | Register holds the address to read from |

---

## Project Structure

```
SoftwareCPUEmulator/
├── include/
│   ├── isa.h            # Opcodes, registers, flags, memory map, encoding macros
│   ├── cpu.h            # CPU struct — registers, PC, SP, flags, cycle count
│   ├── memory.h         # Memory struct — 64KB array + MMIO state
│   ├── alu.h            # ALU result type and all operation signatures
│   └── assembler.h      # Token types, symbol table, assembler context, signatures
│
├── src/
│   ├── main.c           # Entry point — run / debug / dump / trace commands
│   ├── emulator/
│   │   ├── cpu.c        # Fetch–decode–execute loop, stack ops, flag logic
│   │   ├── memory.c     # Byte/word read-write, MMIO handler
│   │   └── alu.c        # All arithmetic and logic operations
│   ├── assembler/
│   │   ├── lexer.c      # Tokeniser — labels, mnemonics, registers, immediates
│   │   └── assembler.c  # Two-pass encoder — writes .bin file
│   └── programs/
│       ├── fibonacci.asm
│       ├── factorial.asm
│       ├── timer.asm
│       └── string.asm
│
├── trace_viewer/        # Web-based execution trace visualiser
│   ├── index.html       # Landing page — drop zone for trace.json and map.json
│   ├── style.css        # Dark theme UI styling
│   └── viewer.js        # Step-through logic — renders registers, flags, timeline
│
├── docs/
│   ├── ISA.md           # Full instruction set reference
│   └── memory_map.md    # Memory layout diagram
│
├── tests/
│   ├── test_alu.c       # Unit tests for every ALU operation
│   ├── test_cpu.c       # Unit tests for fetch–decode–execute
│   └── test_memory.c    # Unit tests for memory read/write and MMIO
│
├── bin/                 # Compiled binaries — gitignored
├── build/               # Assembled .bin outputs and trace files — gitignored
├── Makefile
└── README.md
```

---

## Build Requirements

- GCC or Clang with C11 support
- GNU Make
- macOS or Linux (tested on both)

No external libraries or dependencies.

---

## Quick Start

```bash
# Clone the repo
git clone https://github.com/madhunicabala/SoftwareCPUEmulator.git
cd SoftwareCPUEmulator

# Build the emulator and assembler
make

# Create output folder
mkdir -p build

# Assemble and run a program
./bin/emu16asm src/programs/fibonacci.asm build/fibonacci.bin
./bin/emu16 run build/fibonacci.bin
```

---

## Usage

### Emulator — `emu16`

```
./bin/emu16 run   <program.bin>     Run program until HALT
./bin/emu16 debug <program.bin>     Step-by-step — press Enter to advance, q to quit
./bin/emu16 dump  <program.bin>     Run then dump memory and register state
```

#### Debug mode output example

```
0x2000: [0x08C1]  MOV     dst=R0  mode=1  src=0x01
=== CPU State (cycle 0) ===
  PC=0x2002  SP=0x1FFE
  R0=0x0001 (    1)  R1=0x0000 (    0)  R2=0x0000 (    0)  R3=0x0000 (    0)
  FLAGS: Z=0 N=0 C=0 V=0
>
```

### Assembler — `emu16asm`

```
./bin/emu16asm <source.asm> <output.bin>
```

#### Assembler output example

```
[ASM] Assembling src/programs/fibonacci.asm → build/fibonacci.bin
[ASM] Lexed 24 lines from src/programs/fibonacci.asm
[ASM] Pass1: label 'loop' = 0x200A
[ASM] Pass1: label 'done' = 0x2018
[ASM] Pass 1 complete: 2 labels, 0 errors
[ASM] Pass 2 complete: 28 bytes encoded, 0 errors
[ASM] Wrote 28 bytes to build/fibonacci.bin
[ASM] Done. 28 bytes, 2 labels.
```

---

## Trace Viewer

The trace viewer is a browser-based tool that lets you step through every CPU cycle visually — registers, flags, source line, and instruction detail — one clock at a time.

### How to generate a trace

```bash
# Step 1 — assemble your program
./bin/emu16asm src/programs/factorial.asm build/factorial.bin

# Step 2 — run in trace mode (generates trace.json and map.json)
./bin/emu16 trace build/factorial.bin
```

This produces two files in `build/`:
- `trace.json` — full execution trace, one entry per CPU cycle
- `map.json` — source map linking binary addresses back to assembly lines (optional)

### How to open the viewer

```bash
open trace_viewer/index.html
```

The viewer opens in your browser. Drag and drop `build/trace.json` into the left drop zone, and optionally `build/map.json` into the right drop zone, then click **Open Viewer →**.

### What you can see

- Every register value (R0–R3, PC, SP) at each cycle
- Flag state (Z, N, C, V) after each instruction
- The current instruction being executed with its decoded fields
- A timeline showing how the program progressed cycle by cycle
- Source map overlay showing which assembly line corresponds to the current instruction

---

## Instruction Set

29 instructions across 6 categories. All opcodes are 6 bits wide, allowing up to 64 total instructions.

### System

| Mnemonic | Opcode | Operation |
|----------|--------|-----------|
| NOP | 0x00 | No operation |
| HALT | 0x01 | Stop CPU execution |

### Data Movement

| Mnemonic | Opcode | Operation |
|----------|--------|-----------|
| MOV | 0x02 | `Rd = Rs` or `Rd = imm` |
| LOAD | 0x03 | `Rd = Memory[addr]` |
| STORE | 0x04 | `Memory[addr] = Rs` |
| PUSH | 0x05 | `Memory[--SP] = Rs` |
| POP | 0x06 | `Rd = Memory[SP++]` |

### Arithmetic

| Mnemonic | Opcode | Operation |
|----------|--------|-----------|
| ADD | 0x07 | `Rd = Rd + operand` |
| SUB | 0x08 | `Rd = Rd - operand` |
| MUL | 0x09 | `Rd = Rd * operand` |
| INC | 0x0A | `Rd = Rd + 1` |
| DEC | 0x0B | `Rd = Rd - 1` |

### Logic

| Mnemonic | Opcode | Operation |
|----------|--------|-----------|
| AND | 0x0C | `Rd = Rd & operand` |
| OR | 0x0D | `Rd = Rd \| operand` |
| XOR | 0x0E | `Rd = Rd ^ operand` |
| NOT | 0x0F | `Rd = ~Rd` |
| SHL | 0x10 | `Rd = Rd << imm` |
| SHR | 0x11 | `Rd = Rd >> imm` |

### Compare & Jump

| Mnemonic | Opcode | Operation |
|----------|--------|-----------|
| CMP | 0x12 | Sets flags from `Rd - operand`, result discarded |
| JMP | 0x13 | `PC = addr` unconditional |
| JZ | 0x14 | Jump if Zero flag set |
| JNZ | 0x15 | Jump if Zero flag clear |
| JL | 0x16 | Jump if Negative flag set |
| JGE | 0x17 | Jump if Negative flag clear |
| JC | 0x18 | Jump if Carry flag set |

### Subroutine

| Mnemonic | Opcode | Operation |
|----------|--------|-----------|
| CALL | 0x19 | Push PC onto stack, jump to addr |
| RET | 0x1A | Pop PC from stack, return to caller |

### I/O

| Mnemonic | Opcode | Operation |
|----------|--------|-----------|
| IN | 0x1B | `Rd = MMIO[port]` — read from device |
| OUT | 0x1C | `MMIO[port] = Rs` — write to device |

### Flag Register

Each instruction that produces a result updates the flags:

| Bit | Flag | Set when |
|-----|------|----------|
| 0 | Z — Zero | Result equals zero |
| 1 | N — Negative | Bit 15 of result is set |
| 2 | C — Carry | Result exceeded 16 bits (unsigned overflow) |
| 3 | V — Overflow | Signed overflow occurred |

---

## Memory Map

Total address space: **64KB** (0x0000 – 0xFFFF)

```
0x0000 ┌──────────────────────┐
       │   Data Segment       │  4 KB  — global variables, strings
0x0FFF └──────────────────────┘
0x1000 ┌──────────────────────┐
       │   Stack Segment      │  4 KB  — grows downward
       │                      │          SP initialises to 0x1FFF
0x1FFF └──────────────────────┘
0x2000 ┌──────────────────────┐
       │   Code Segment       │  52 KB — program instructions
       │                      │          PC initialises to 0x2000
0xEFFF └──────────────────────┘
0xF000 ┌──────────────────────┐
       │   MMIO               │  256 B — timer and character I/O
0xF0FF └──────────────────────┘
0xF100 ┌──────────────────────┐
       │   Reserved           │
0xFFFF └──────────────────────┘
```

### MMIO Ports

| Address | Name | Direction | Purpose |
|---------|------|-----------|---------|
| 0xF000 | TIMER_COUNT | Read | Current timer tick count |
| 0xF001 | TIMER_CTRL | Write | `1` = start timer, `0` = stop and reset |
| 0xF002 | STDOUT | Write | Write one character to terminal |
| 0xF003 | STDIN | Read | Read one character from terminal |

---

## Calling Convention

Defines how functions pass arguments, return values, and manage the stack.

### Register Roles

| Register | Role | Saved by |
|----------|------|----------|
| R0 | Argument 1 / Return value | Caller |
| R1 | Argument 2 / Scratch | Caller |
| R2 | Argument 3 / Scratch | Caller |
| R3 | Frame Pointer (FP) | Callee |
| SP | Stack Pointer | Hardware |
| PC | Program Counter | Hardware |

### Stack Frame Layout

```
Higher addresses
┌──────────────────────────┐  ← SP before CALL
│   Return Address (PC)    │  pushed automatically by CALL
├──────────────────────────┤
│   Saved R3 (old FP)      │  pushed by callee prologue
├──────────────────────────┤
│   Local variables        │
└──────────────────────────┘  ← current SP
Lower addresses
```

### Function Call Pattern

```asm
; Caller
MOV  R0, #5          ; argument 1
CALL factorial       ; push return addr, jump

; Callee prologue
factorial:
PUSH R3              ; save old frame pointer
MOV  R3, SP          ; set new frame pointer

; ... function body ...

; Callee epilogue
POP  R3              ; restore old frame pointer
RET                  ; pop return address, jump back
```

---

## Assembler

The assembler (`emu16asm`) converts human-readable `.asm` source files into `.bin` machine code that the emulator can load and execute. It is implemented across three files.

### How it works

**`include/assembler.h`** defines all shared types — `Token`, `TokenType`, `ParsedLine`, `Symbol`, `SymbolTable`, and `AsmContext` — used by both the lexer and encoder.

**`src/assembler/lexer.c`** handles lexing. It reads the `.asm` file line by line, strips comments, detects label definitions, and classifies each word into a typed `Token`: mnemonic, register, immediate, direct address, indirect address, or label reference.

**`src/assembler/assembler.c`** runs the two passes:
- **Pass 1** walks every `ParsedLine`, records each `label → address` into the symbol table, and advances an address counter by the correct instruction size so forward label references resolve correctly.
- **Pass 2** encodes each instruction to a 16-bit binary word using the `MAKE_INSTR` macro from `isa.h`, appends an extra 16-bit address word for `DIRECT` mode instructions, and writes the final binary to a `.bin` file.

### Assembly syntax

```asm
; This is a comment — ignored by the lexer

        MOV  R0, #0         ; immediate: # prefix, range -32 to +31
        MOV  R1, R2         ; register to register
        LOAD R0, [0x0010]   ; direct memory address
        LOAD R0, [R1]       ; indirect — address held in R1
        STORE R0, [0x0010]  ; store register to memory address

loop:                       ; label definition
        INC  R0
        CMP  R0, #10
        JNZ  loop           ; label reference as jump target

        OUT  R0, [0xF002]   ; write character to MMIO STDOUT
        HALT
```

### Two-word instructions

These instructions emit two 16-bit words — the instruction word followed by a full 16-bit address word:

| Instructions | Why |
|---|---|
| `JMP`, `JZ`, `JNZ`, `JL`, `JGE`, `JC`, `CALL` | Jump/call target needs full 16-bit address |
| `LOAD Rd, [addr]` / `STORE Rs, [addr]` | Direct memory address |
| `IN Rd, [port]` / `OUT Rd, [port]` | MMIO port address |

---

## Example Programs

All four programs have been assembled and verified on the emulator.

### Fibonacci

Computes the Fibonacci sequence iteratively up to the 10th term.

```bash
./bin/emu16asm src/programs/fibonacci.asm build/fibonacci.bin
./bin/emu16 run build/fibonacci.bin
```

```
[CPU] Loaded 28 bytes at 0x2000
[CPU] HALT at cycle 84, PC=0x201A
[CPU] Finished in 85 cycles

=== CPU State (cycle 85) ===
  PC=0x201C  SP=0x1FFF
  R0=0x0037 (   55)    R1=0x0059 (   89)    R2=0x0000 (    0)    R3=0x0014 (   20)
  FLAGS: Z=1 N=0 C=0 V=0
```

R0 = 55 = fib(10)  &nbsp; R1 = 89 = fib(11) 

---

### Factorial

Computes 5! recursively using CALL/RET and stack frames.

```bash
./bin/emu16asm src/programs/factorial.asm build/factorial.bin
./bin/emu16 run build/factorial.bin
```

```
[CPU] Loaded 36 bytes at 0x2000
[CPU] HALT at cycle 42, PC=0x2006
[CPU] Finished in 43 cycles

=== CPU State (cycle 43) ===
  PC=0x2008  SP=0x1FFF
  R0=0x0078 (  120)    R1=0x0005 (    5)    R2=0x0000 (    0)    R3=0x0000 (    0)
  FLAGS: Z=0 N=0 C=0 V=0
```

R0 = 120 = 5!  &nbsp; Stack fully unwound (SP = 0x1FFF) 

---

### Timer

Starts the MMIO timer, polls it in a loop for 20 ticks, reads the count back.

```bash
./bin/emu16asm src/programs/timer.asm build/timer.bin
./bin/emu16 run build/timer.bin
```

```
[CPU] Loaded 38 bytes at 0x2000
[CPU] HALT at cycle 79, PC=0x2024
[CPU] Finished in 80 cycles

=== CPU State (cycle 80) ===
  PC=0x2026  SP=0x1FFF
  R0=0x0000 (    0)    R1=0x0014 (   20)    R2=0x0000 (    0)    R3=0x0000 (    0)
  FLAGS: Z=1 N=0 C=0 V=0
```

R1 = 20 ticks counted!  &nbsp; Z=1 confirms loop exited cleanly 

---

### String

Prints `Hello, Emu16!` character by character via MMIO STDOUT. Each ASCII value is built using SHL + ADD since the 6-bit immediate field is limited to −32 to +31.

```bash
./bin/emu16asm src/programs/string.asm build/string.bin
./bin/emu16 run build/string.bin
```

```
[CPU] Loaded 128 bytes at 0x2000
Hello, Emu16!
[CPU] HALT at cycle 49, PC=0x207E
[CPU] Finished in 50 cycles

=== CPU State (cycle 50) ===
  PC=0x2080  SP=0x1FFF
  R0=0x000A (   10)    R1=0x0000 (    0)    R2=0x0000 (    0)    R3=0x0000 (    0)
  FLAGS: Z=0 N=0 C=0 V=0
```

`Hello, Emu16!` printed to terminal! &nbsp; R0 = 10 = '\n' (last char written) 

---

## Demo Video - Fibonacci Program
https://drive.google.com/file/d/1QtM5ycDp4ezod9oRnGZtgOsxTnU_7PVH/view?usp=drive_link 

## Team

| Member | Contribution |
|--------|-------------|
| Madhunica Balasubramanian | ISA architecture, CPU design constraints, ISA design, emulator (cpu.c, memory.c, alu.c) |
| Ekant Kapgate | Assembler (assembler.h, lexer.c, assembler.c) |
| Prathamesh Ravindra Sawant| Assembly programs (fibonacci.asm, factorial.asm, timer.asm, string.asm) |
| Siddhant Raje | Tests (test_alu.c, test_cpu.c, test_memory.c), README, project report |

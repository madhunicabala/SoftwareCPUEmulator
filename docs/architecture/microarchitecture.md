# SoftCPU-C Microarchitecture

## Overview

SoftCPU-C is a 16-bit software-emulated CPU with a fixed-width 2-byte (16-bit) instruction word, 4 general-purpose registers, a hardware stack, memory-mapped I/O, and a simple three-stage pipeline model: **Fetch → Decode → Execute**.

---

## Internal Registers

| Register | Width | Init value | Purpose |
|----------|-------|-----------|---------|
| **PC** | 16-bit | `0x2000` | Program Counter — address of next instruction to fetch |
| **MAR** | 16-bit | — | Memory Address Register — holds address sent to the memory bus |
| **MDR** | 16-bit | — | Memory Data Register — holds data returned from (or sent to) memory |
| **IR** | 16-bit | — | Instruction Register — holds the currently executing instruction word |
| **SP** | 16-bit | `0x1FFF` | Stack Pointer — grows downward toward `0x1000` |
| **FLAGS** | 3-bit | `0b000` | Status flags: Z (zero), N (negative/less), C (carry) |
| **R0–R3** | 16-bit | `0` | General-purpose registers (calling convention: R0=arg1/ret, R1=arg2, R2=arg3, R3=frame ptr) |

---

## Instruction Format

Every instruction is a single 16-bit word:

```
 15    10  9   8  7   6  5        0
 ┌──────┬────┬────┬────────────────┐
 │opcode│mode│ dst│   src / imm    │
 │ [6b] │[2b]│[2b]│     [6b]      │
 └──────┴────┴────┴────────────────┘
```

**Addressing modes** (`mode` field):

| Mode | Value | Meaning |
|------|-------|---------|
| REG | `00` | `src` field is a register number (R0–R3) |
| IMM | `01` | `src` field is a 6-bit signed immediate (−32 to +31) |
| DIRECT | `10` | Next 16-bit word in the instruction stream is the address/value |
| INDIRECT | `11` | `src` field is a register; effective address = `reg[src]` |

`MOVW` is the only 4-byte instruction: the second 16-bit word carries a full 16-bit immediate or label address.

---

## Fetch → Decode → Execute Cycle

```
┌──────────────────────────────────────────────────────────┐
│  FETCH                                                   │
│    MAR  ← PC                                             │
│    MDR  ← mem[MAR]          (read instruction word)      │
│    PC   ← PC + 2            (advance to next word)       │
└──────────────────────────────┬───────────────────────────┘
                               │
┌──────────────────────────────▼───────────────────────────┐
│  DECODE                                                  │
│    IR   ← MDR               (latch instruction)          │
│    opcode ← IR[15:10]                                    │
│    mode   ← IR[9:8]                                      │
│    dst    ← IR[7:6]                                      │
│    src    ← IR[5:0]                                      │
└──────────────────────────────┬───────────────────────────┘
                               │
┌──────────────────────────────▼───────────────────────────┐
│  EXECUTE                                                 │
│    Resolve operand per mode (REG/IMM/DIRECT/INDIRECT)    │
│    ALU computes result                                   │
│    Update FLAGS (Z, N, C) if arithmetic/logic/CMP        │
│    Writeback to dst register or memory                   │
│    Branch: if flag condition true → PC ← target address  │
└──────────────────────────────────────────────────────────┘
```

For MMIO (`IN`/`OUT`), the execute stage intercepts addresses in `0xF000–0xF0FF` and routes them to the hardware device layer instead of main memory.

---

## Memory Map

```
0x0000 ┌──────────────────────────────┐
       │         DATA  (4 KB)         │ ← LOAD / STORE targets
0x0FFF ├──────────────────────────────┤
0x1000 │         STACK  (4 KB)        │ ← SP starts at 0x1FFF, grows ↓
0x1FFF ├──────────────────────────────┤
0x2000 │         CODE  (52 KB)        │ ← PC starts here; read-only
0xEFFF ├──────────────────────────────┤
0xF000 │         MMIO  (256 B)        │ ← IN / OUT instructions
0xF0FF └──────────────────────────────┘
```

---

## FLAGS Register

| Bit | Name | Set when |
|-----|------|---------|
| 0 | **Z** (Zero) | Result == 0 |
| 1 | **N** (Negative / Less) | Result < 0 (MSB set) or CMP dst < src |
| 2 | **C** (Carry) | Unsigned overflow on ADD/SUB |

Branch instructions that test flags: `JZ`, `JNZ`, `JL`, `JGE`, `JC`.

---

## MMIO Ports

| Address | Name | Direction | Description |
|---------|------|-----------|-------------|
| `0xF000` | TIMER_COUNT | Read | Current timer tick count (increments each `cpu_step()`) |
| `0xF001` | TIMER_CTRL | Write | Write `1` to start; write `0` to stop and reset |
| `0xF002` | STDOUT | Write | Write a character byte to console output |
| `0xF003` | STDIN | Read | Read next character byte from console input |

---

## Instruction Set Summary

31 opcodes, 6-bit opcode field (`0x00`–`0x1E`):

```
00 NOP    01 HALT   02 MOV    03 LOAD   04 STORE
05 PUSH   06 POP    07 ADD    08 SUB    09 MUL
0A INC    0B DEC    0C AND    0D OR     0E XOR
0F NOT    10 SHL    11 SHR    12 CMP
13 JMP    14 JZ     15 JNZ    16 JL     17 JGE   18 JC
19 CALL   1A RET
1B IN     1C OUT
1D LOADB  1E MOVW
```

---

## Calling Convention

| Register | Role | Save obligation |
|----------|------|----------------|
| R0 | First argument / return value | Caller-saved |
| R1 | Second argument / scratch | Caller-saved |
| R2 | Third argument / scratch | Caller-saved |
| R3 | Frame pointer | **Callee-saved** |
| SP | Stack pointer | Callee must restore |

`CALL label` pushes return address (PC) onto stack and jumps. `RET` pops return address into PC.

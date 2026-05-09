# Recursive Program — Factorial

This directory contains a simple C program that computes `5!` recursively.
Its purpose is to show **how function calls and recursion are handled** on the
Emu16 CPU: the C source is the human-readable specification, and
[`src/programs/factorial.asm`](../src/programs/factorial.asm) is its direct
translation into Emu16 assembly.

---

## Files

| File | Purpose |
|------|---------|
| `factorial.c` | Recursive factorial function |
| `main.c` | Driver — calls `factorial(5)` and prints the result |

---

## Build and Run

```bash
gcc -Wall -o factorial main.c factorial.c
./factorial
```

Expected output:

```
5! = 120
```

---

## How It Maps to the Emu16 Assembly

The C code translates almost line-for-line into the assembly in
`src/programs/factorial.asm`:

| C | Emu16 assembly |
|---|----------------|
| `int n = 5;` | `MOV R0, #5` |
| `factorial(n)` | `CALL factorial` |
| `if (n <= 1) return 1;` | `CMP R0, #1` / `JZ base_case` / `JL base_case` |
| `return n * factorial(n-1);` | `PUSH R0` / `DEC R0` / `CALL factorial` / `POP R1` / `MUL R0, R1` / `RET` |

### Calling Convention

The Emu16 uses a simple caller/callee convention:

| Register | Role |
|----------|------|
| R0 | First argument **and** return value |
| R1 | Second argument / scratch (caller-saved) |
| R2 | Third argument / scratch (caller-saved) |
| R3 | Frame pointer (callee-saved) |
| SP | Stack pointer — grows **downward** from `0x1FFF` |

`CALL label` pushes the return address (current PC) onto the stack and jumps.  
`RET` pops that address back into PC.

---

## Memory Layout

The Emu16 splits its 64 KB address space into four segments:

```
0x0000 ┌──────────────────────────────┐
       │         DATA  (4 KB)         │  ← LOAD / STORE targets
0x0FFF ├──────────────────────────────┤
0x1000 │         STACK  (4 KB)        │  ← SP starts at 0x1FFF, grows ↓
0x1FFF ├──────────────────────────────┤
0x2000 │         CODE  (52 KB)        │  ← PC starts here; factorial.bin loaded here
0xEFFF ├──────────────────────────────┤
0xF000 │         MMIO  (256 B)        │  ← IN / OUT ports
0xF0FF └──────────────────────────────┘
```

When `factorial.bin` is loaded and run:

- PC is set to `0x2000` and the CPU starts fetching instructions.
- The `_start` block (`MOV R0, #5` / `CALL factorial`) lives at `0x2000`.
- The `factorial` function body lives just after it in the code segment.
- The stack segment (`0x1000–0x1FFF`) is used for return addresses and saved registers.

---

## How Recursion Is Carried Out — Stack Trace for factorial(5)

Each call to `factorial` does three things on the stack:

1. **CALL** — the CPU automatically pushes the return address.
2. **PUSH R0** — the function saves the current `n` before decrementing.
3. On return: **POP R1** restores `n`, **MUL** computes the partial result, **RET** pops the return address.

Below is the full stack state at every step. Each word on the stack is 2 bytes;
SP starts at `0x1FFF` and decrements by 2 on every push.

```
_start:  MOV R0, #5
_start:  CALL factorial        → push ret=0x2006
         SP=0x1FFE  stack: [0x2006]

┌─ factorial(5)
│  PUSH R0=5                   → save n
│        SP=0x1FFC  stack: [0x2006, 5]
│  CALL factorial(4)           → push ret=0x2012
│        SP=0x1FFA  stack: [0x2006, 5, 0x2012]
│
│  ┌─ factorial(4)
│  │  PUSH R0=4
│  │        SP=0x1FF8  stack: [0x2006, 5, 0x2012, 4]
│  │  CALL factorial(3)        → push ret=0x2012
│  │        SP=0x1FF6  stack: [0x2006, 5, 0x2012, 4, 0x2012]
│  │
│  │  ┌─ factorial(3)
│  │  │  PUSH R0=3
│  │  │        SP=0x1FF4  stack: [0x2006, 5, 0x2012, 4, 0x2012, 3]
│  │  │  CALL factorial(2)     → push ret=0x2012
│  │  │        SP=0x1FF2  stack: [0x2006, 5, 0x2012, 4, 0x2012, 3, 0x2012]
│  │  │
│  │  │  ┌─ factorial(2)
│  │  │  │  PUSH R0=2
│  │  │  │        SP=0x1FF0  stack: [..., 3, 0x2012, 2]
│  │  │  │  CALL factorial(1)  → push ret=0x2012
│  │  │  │        SP=0x1FEE  stack: [..., 3, 0x2012, 2, 0x2012]
│  │  │  │
│  │  │  │  ┌─ factorial(1)   ← base case
│  │  │  │  │  MOV R0, #1
│  │  │  │  └─ RET            → pop 0x2012,  R0=1
│  │  │  │        SP=0x1FF0  stack: [..., 3, 0x2012, 2]
│  │  │  │
│  │  │  │  POP  R1=2         ← restore saved n
│  │  │  │  MUL  R0=1 * R1=2  →  R0=2
│  │  │  └─ RET               → pop 0x2012,  R0=2
│  │  │        SP=0x1FF4  stack: [0x2006, 5, 0x2012, 4, 0x2012, 3]
│  │  │
│  │  │  POP  R1=3
│  │  │  MUL  R0=2 * R1=3  →  R0=6
│  │  └─ RET                  → pop 0x2012,  R0=6
│  │        SP=0x1FF8  stack: [0x2006, 5, 0x2012, 4]
│  │
│  │  POP  R1=4
│  │  MUL  R0=6 * R1=4  →  R0=24
│  └─ RET                     → pop 0x2012,  R0=24
│        SP=0x1FFC  stack: [0x2006, 5]
│
│  POP  R1=5
│  MUL  R0=24 * R1=5  →  R0=120
└─ RET                        → pop 0x2006,  R0=120
         SP=0x2000  stack: (empty)

_start:  HALT  →  R0 = 120
```

### Key observations

- **Stack depth at maximum recursion (n=1):** 9 words / 18 bytes, SP = `0x1FEE`.
- **Each frame** holds exactly two words: the saved `n` (from `PUSH R0`) and
  the return address (from `CALL`). No heap, no local variables.
- **Unwinding is symmetric:** every `PUSH` on the way down has a matching `POP`
  on the way up; every `CALL` has a matching `RET`.
- **R0 is both the argument and the return value.** The caller passes `n` in R0;
  the callee overwrites R0 with the result before `RET`.

---

## Running on the Emu16 Emulator

To assemble and run the assembly version on the actual Emu16 CPU emulator:

```bash
# From the repo root
./bin/softasm src/programs/factorial.asm build/factorial.bin
./bin/softcpu run build/factorial.bin
```

The emulator halts with `R0 = 120` — the same answer the C program prints.

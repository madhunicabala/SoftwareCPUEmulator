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
| `factorial.c` | Recursive factorial function — mirrors the Emu16 calling convention |
| `main.c` | Driver — calls `factorial(5)` and prints the result |

---

## Build and Run

### C Program

```bash
# From the recursive_program/ directory
gcc -Wall -o factorial main.c factorial.c
./factorial
```

Expected output:

```
5! = 120
```

### Emu16 Assembly Version

```bash
# From the repo root
./bin/emu16asm src/programs/factorial.asm build/factorial.bin
./bin/emu16 run build/factorial.bin
```

Expected output:

```
[CPU] Loaded 36 bytes at 0x2000
[CPU] HALT at cycle 42, PC=0x2006
[CPU] Finished in 43 cycles

=== CPU State (cycle 43) ===
  PC=0x2008  SP=0x1FFF
  R0=0x0078 (  120)    R1=0x0005 (    5)
  FLAGS: Z=0 N=0 C=0 V=0
```

R0 = 120 = 5! ✅ — Stack fully unwound (SP = 0x1FFF) ✅

### Step Through Execution (Debug Mode)

```bash
./bin/emu16 debug build/factorial.bin
```

Press **Enter** to advance one instruction at a time. Type **q** to quit.
Watch the stack pointer (SP) drop with each CALL and PUSH, and climb back
with each POP and RET.

---

## How It Maps to Emu16 Assembly

The C code translates almost line-for-line into the assembly in
`src/programs/factorial.asm`:

| C | Emu16 Assembly |
|---|----------------|
| `int n = 5;` | `MOV R0, #5` |
| `factorial(n)` | `CALL factorial` |
| `if (n <= 1) return 1;` | `CMP R0, #1` / `JZ base_case` / `JL base_case` |
| `return n * factorial(n-1);` | `PUSH R0` / `DEC R0` / `CALL factorial` / `POP R1` / `MUL R0, R1` / `RET` |

---

## Calling Convention

The Emu16 uses a simple caller/callee convention that this program exercises fully:

| Register | Role | Saved by |
|----------|------|----------|
| R0 | First argument **and** return value | Caller |
| R1 | Second argument / scratch | Caller |
| R2 | Third argument / scratch | Caller |
| R3 | Frame pointer | Callee |
| SP | Stack pointer — grows **downward** from `0x1FFF` | Hardware |

`CALL label` — pushes the return address (current PC) onto the stack and jumps.
`RET` — pops that address back into PC and returns to the caller.

---

## Memory Layout

The Emu16 splits its 64KB address space into four segments:

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

When `factorial.bin` is loaded and executed:

- **PC** is set to `0x2000` — the CPU starts fetching here.
- The `_start` block (`MOV R0, #5` / `CALL factorial`) lives at `0x2000`.
- The `factorial` function body lives just after it in the code segment.
- The stack segment (`0x1000–0x1FFF`) holds return addresses and saved `n` values during recursion.
- The data segment and MMIO are not used by this program.

---

## How Recursion Is Carried Out — Stack Trace for factorial(5)

Each recursive call to `factorial` does exactly two things on the stack:

1. **`CALL`** — the CPU automatically pushes the return address (2 bytes).
2. **`PUSH R0`** — the function saves the current value of `n` before decrementing (2 bytes).

Each frame is therefore exactly **4 bytes** (2 words). On return:
- **`POP R1`** restores the saved `n`.
- **`MUL R0, R1`** computes the partial product.
- **`RET`** pops the return address and jumps back to the caller.

```
_start:  MOV R0, #5
_start:  CALL factorial        → push ret_addr=0x2006
         SP=0x1FFD  stack: [0x2006]

┌─ factorial(5)
│  PUSH R0=5                   → save n=5
│        SP=0x1FFB  stack: [0x2006, 5]
│  CALL factorial(4)           → push ret_addr=0x2012
│        SP=0x1FF9  stack: [0x2006, 5, 0x2012]
│
│  ┌─ factorial(4)
│  │  PUSH R0=4                → save n=4
│  │        SP=0x1FF7  stack: [0x2006, 5, 0x2012, 4]
│  │  CALL factorial(3)        → push ret_addr=0x2012
│  │        SP=0x1FF5  stack: [0x2006, 5, 0x2012, 4, 0x2012]
│  │
│  │  ┌─ factorial(3)
│  │  │  PUSH R0=3             → save n=3
│  │  │        SP=0x1FF3  stack: [..., 4, 0x2012, 3]
│  │  │  CALL factorial(2)     → push ret_addr=0x2012
│  │  │        SP=0x1FF1  stack: [..., 4, 0x2012, 3, 0x2012]
│  │  │
│  │  │  ┌─ factorial(2)
│  │  │  │  PUSH R0=2          → save n=2
│  │  │  │        SP=0x1FEF  stack: [..., 3, 0x2012, 2]
│  │  │  │  CALL factorial(1)  → push ret_addr=0x2012
│  │  │  │        SP=0x1FED  stack: [..., 3, 0x2012, 2, 0x2012]
│  │  │  │
│  │  │  │  ┌─ factorial(1)   ← BASE CASE
│  │  │  │  │  CMP R0, #1     → Z flag set
│  │  │  │  │  MOV R0, #1     → return value = 1
│  │  │  │  └─ RET            → pop 0x2012, jump back
│  │  │  │        SP=0x1FEF  stack: [..., 3, 0x2012, 2]
│  │  │  │
│  │  │  │  POP  R1=2         ← restore saved n=2
│  │  │  │  MUL  R0=1 × 2  →  R0=2
│  │  │  └─ RET               → pop 0x2012, jump back
│  │  │        SP=0x1FF3  stack: [..., 4, 0x2012, 3]
│  │  │
│  │  │  POP  R1=3            ← restore saved n=3
│  │  │  MUL  R0=2 × 3  →  R0=6
│  │  └─ RET                  → pop 0x2012, jump back
│  │        SP=0x1FF7  stack: [..., 5, 0x2012, 4]
│  │
│  │  POP  R1=4               ← restore saved n=4
│  │  MUL  R0=6 × 4  →  R0=24
│  └─ RET                     → pop 0x2012, jump back
│        SP=0x1FFB  stack: [0x2006, 5]
│
│  POP  R1=5                  ← restore saved n=5
│  MUL  R0=24 × 5  →  R0=120
└─ RET                        → pop 0x2006, jump back to _start
         SP=0x1FFF  stack: (empty — fully unwound)

_start:  HALT  →  R0 = 120  ✅
```

### Key Observations

- **Maximum stack depth** (at base case n=1): 8 words / 16 bytes, SP = `0x1FED`
- **Each frame** holds exactly 2 words: saved `n` (from `PUSH R0`) and the return address (from `CALL`)
- **Unwinding is perfectly symmetric** — every `PUSH` going down has a matching `POP` coming up; every `CALL` has a matching `RET`
- **R0 serves as both argument and return value** — the caller passes `n` in R0; the callee overwrites R0 with the result before `RET`
- **No heap, no local variables** — the entire recursive computation uses only the stack and registers

---

## Trace Viewer

To visualise the recursion cycle by cycle in the browser:

```bash
./bin/emu16 trace build/factorial.bin
open trace_viewer/index.html
```

Drop `build/factorial.trace.json` into the viewer and step through each cycle to
watch the stack pointer drop during recursion and climb back during unwinding.

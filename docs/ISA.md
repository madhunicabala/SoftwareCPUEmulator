# SoftCPU-C Instruction Set Architecture

## Instruction Encoding (16-bit fixed width)

| Bits  | Field      | Size   | Description                  |
|-------|------------|--------|------------------------------|
| 15–10 | OPCODE     | 6 bits | Up to 64 instructions        |
| 9–8   | MODE       | 2 bits | Addressing mode              |
| 7–6   | DST        | 2 bits | Destination register R0–R3   |
| 5–0   | SRC / IMM  | 6 bits | Source register or immediate |

## Addressing Modes

| Code | Name     | Meaning                          |
|------|----------|----------------------------------|
| 00   | REG      | Operand is a register            |
| 01   | IMM      | 6-bit sign-extended immediate    |
| 10   | DIRECT   | Next word is full 16-bit address |
| 11   | INDIRECT | Register holds the address       |

## Registers

| Name | Role                              |
|------|-----------------------------------|
| R0   | Argument 1 / Return value         |
| R1   | Argument 2 / Scratch              |
| R2   | Argument 3 / Scratch              |
| R3   | Frame Pointer (callee-saved)      |
| SP   | Stack Pointer (special)           |
| PC   | Program Counter (special)         |

## Instruction Table

See include/isa.h for full opcode values.

| Mnemonic | Opcode | Operation                          |
|----------|--------|------------------------------------|
| NOP      | 0x00   | No operation                       |
| HALT     | 0x01   | Stop CPU                           |
| MOV      | 0x02   | Rd = Rs / imm                      |
| LOAD     | 0x03   | Rd = Memory[addr]                  |
| STORE    | 0x04   | Memory[addr] = Rs                  |
| PUSH     | 0x05   | Memory[--SP] = Rs                  |
| POP      | 0x06   | Rd = Memory[SP++]                  |
| ADD      | 0x07   | Rd = Rd + operand                  |
| SUB      | 0x08   | Rd = Rd - operand                  |
| MUL      | 0x09   | Rd = Rd * operand                  |
| INC      | 0x0A   | Rd = Rd + 1                        |
| DEC      | 0x0B   | Rd = Rd - 1                        |
| AND      | 0x0C   | Rd = Rd & operand                  |
| OR       | 0x0D   | Rd = Rd \| operand                 |
| XOR      | 0x0E   | Rd = Rd ^ operand                  |
| NOT      | 0x0F   | Rd = ~Rd                           |
| SHL      | 0x10   | Rd = Rd << imm                     |
| SHR      | 0x11   | Rd = Rd >> imm                     |
| CMP      | 0x12   | flags = Rd - operand (no store)    |
| JMP      | 0x13   | PC = addr                          |
| JZ       | 0x14   | PC = addr if Z                     |
| JNZ      | 0x15   | PC = addr if !Z                    |
| JL       | 0x16   | PC = addr if N                     |
| JGE      | 0x17   | PC = addr if !N                    |
| JC       | 0x18   | PC = addr if C                     |
| CALL     | 0x19   | push PC; PC = addr                 |
| RET      | 0x1A   | PC = pop()                         |
| IN       | 0x1B   | Rd = MMIO[port]                    |
| OUT      | 0x1C   | MMIO[port] = Rs                    |

## Memory Map

| Range           | Segment | Size  | Purpose              |
|-----------------|---------|-------|----------------------|
| 0x0000 – 0x0FFF | Data    | 4 KB  | Globals, strings     |
| 0x1000 – 0x1FFF | Stack   | 4 KB  | Grows downward       |
| 0x2000 – 0xEFFF | Code    | 52 KB | Program instructions |
| 0xF000 – 0xF0FF | MMIO    | 256 B | Timer, I/O ports     |

## MMIO Ports

| Address | Name         | Direction | Purpose             |
|---------|--------------|-----------|---------------------|
| 0xF000  | TIMER_COUNT  | Read      | Current tick count  |
| 0xF001  | TIMER_CTRL   | Write     | 1=start, 0=stop     |
| 0xF002  | STDOUT       | Write     | Print character     |
| 0xF003  | STDIN        | Read      | Read character      |

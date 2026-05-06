; hello.asm — Print "Hello, World!" to MMIO STDOUT (0xF002)
;
; Layout in code segment (PC starts at 0x2000):
;   0x2000  JMP start          — skip over the string data (4 bytes)
;   0x2004  msg: .string ...   — 14 bytes ("Hello, World!" + null) = 14 (even, no pad)
;   0x2012  start:             — actual program code
;
; New instructions used:
;   MOVW  Rd, label  — load 16-bit label address into register (4 bytes)
;   LOADB Rd, [Rs]   — load one byte from memory at address in Rs (2 bytes)
;
; Demonstrates: data embedding, byte I/O, loop control, MMIO write

        JMP   start            ; skip the string data

msg:    .string "Hello, World!"  ; 13 chars + null = 14 bytes (aligned)

start:
        MOVW  R3, msg          ; R3 = address of string (0x2004)

loop:
        LOADB R0, [R3]         ; R0 = next character byte
        CMP   R0, #0           ; test for null terminator
        JZ    done
        OUT   R0, [0xF002]     ; write character to MMIO STDOUT
        INC   R3               ; advance to next byte
        JMP   loop

done:
        HALT

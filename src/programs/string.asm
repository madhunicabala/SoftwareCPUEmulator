; string.asm — Copy a string from code segment to data segment, then print it
;
; Demonstrates: LOADB (byte load), STORE (word store to data segment),
;               MOVW (16-bit address load), INC, CMP, MMIO STDOUT
;
; Memory layout after run:
;   Code segment 0x2000: JMP start (skip data)
;   Code segment 0x2004: src_str — "Hello!" null-terminated
;   Data segment 0x0000: destination buffer (copied string lands here)

        JMP   start

src_str: .string "Hello!"        ; 6 chars + null = 7 bytes → padded to 8

start:
        ; ── Phase 1: copy src_str → data segment ─────────────────────
        MOVW  R0, src_str        ; R0 = source address (in code segment)
        MOV   R1, #0             ; R1 = destination address (data segment base)

copy_loop:
        LOADB R2, [R0]           ; R2 = byte at src
        CMP   R2, #0             ; check for null terminator
        JZ    print_phase
        STORE R2, [R1]           ; write byte to dest (as word — low byte only)
        INC   R0                 ; advance source pointer
        INC   R1                 ; advance dest pointer
        JMP   copy_loop

        ; ── Phase 2: print from data segment via MMIO STDOUT ─────────
print_phase:
        MOV   R1, #0             ; reset dest pointer to start of data segment

print_loop:
        LOADB R2, [R1]           ; R2 = byte from data segment
        CMP   R2, #0             ; stop at null
        JZ    done
        OUT   R2, [0xF002]       ; write character to MMIO STDOUT
        INC   R1
        JMP   print_loop

done:
=======
; string.asm
; Prints "Hello, SoftCPU!\n" character by character via MMIO STDOUT.
;
; Problem: our 6-bit signed immediate only covers -32 to +31.
; ASCII characters are 32-127, so we can't MOV R0, #72 directly.
;
; Solution: build each character value using SHL + ADD:
;   MOV R0, #18   ; load small base
;   SHL R0, #2    ; shift left: 18 << 2 = 72
;   ADD R0, #0    ; add remainder (none needed here)
;   OUT [0xF002], R0   ; print it
;
; Registers:
;   R0 — current character being built and printed
; ============================================================

; 'H' = 72 = 18 << 2
        MOV   R0, #18
        SHL   R0, #2
        OUT   R0, [0xF002]

; 'e' = 101 = 25 << 2 + 1
        MOV   R0, #25
        SHL   R0, #2
        ADD   R0, #1
        OUT   R0, [0xF002]

; 'l' = 108 = 27 << 2
        MOV   R0, #27
        SHL   R0, #2
        OUT   R0, [0xF002]

; 'l' = 108 = 27 << 2
        MOV   R0, #27
        SHL   R0, #2
        OUT   R0, [0xF002]

; 'o' = 111 = 27 << 2 + 3
        MOV   R0, #27
        SHL   R0, #2
        ADD   R0, #3
        OUT   R0, [0xF002]

; ',' = 44 = 22 << 1
        MOV   R0, #22
        SHL   R0, #1
        OUT   R0, [0xF002]

; ' ' = 32 = 16 << 1
        MOV   R0, #16
        SHL   R0, #1
        OUT   R0, [0xF002]

; 'S' = 83 = 20 << 2 + 3
        MOV   R0, #20
        SHL   R0, #2
        ADD   R0, #3
        OUT   R0, [0xF002]

; 'o' = 111 = 27 << 2 + 3
        MOV   R0, #27
        SHL   R0, #2
        ADD   R0, #3
        OUT   R0, [0xF002]

; 'f' = 102 = 25 << 2 + 2
        MOV   R0, #25
        SHL   R0, #2
        ADD   R0, #2
        OUT   R0, [0xF002]

; 't' = 116 = 29 << 2
        MOV   R0, #29
        SHL   R0, #2
        OUT   R0, [0xF002]

; 'C' = 67 = 16 << 2 + 3
        MOV   R0, #16
        SHL   R0, #2
        ADD   R0, #3
        OUT   R0, [0xF002]

; 'P' = 80 = 20 << 2
        MOV   R0, #20
        SHL   R0, #2
        OUT   R0, [0xF002]

; 'U' = 85 = 21 << 2 + 1
        MOV   R0, #21
        SHL   R0, #2
        ADD   R0, #1
        OUT   R0, [0xF002]

; '!' = 33 = 16 << 1 + 1
        MOV   R0, #16
        SHL   R0, #1
        ADD   R0, #1
        OUT   R0, [0xF002]

; '\n' = 10 = 5 << 1
        MOV   R0, #5
        SHL   R0, #1
        OUT   R0, [0xF002]

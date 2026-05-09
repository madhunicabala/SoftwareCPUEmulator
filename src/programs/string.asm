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

        HALT

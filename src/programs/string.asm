; string.asm
; Demonstrates string operations: store a string in the data segment,
; walk it character by character, print each via MMIO STDOUT.
;
; What it does:
;   1. Stores "Hello, SoftCPU!\n" in the data segment at 0x0000
;   2. Loads each character one by one using indirect addressing
;   3. Writes each character to MMIO STDOUT (0xF002)
;   4. Stops when it hits the null terminator (0x00)
;
; Registers used:
;   R0 — current character value
;   R1 — current pointer address (walks through string)
;   R2 — scratch / null check
; ============================================================

; ── Store string into data segment ──────────────────────────
; "Hello, SoftCPU!\n\0"
; We store each character as a word (16-bit) at consecutive
; even addresses starting at 0x0000

        MOV   R0, #72          ; 'H'
        STORE R0, [0x0000]
        MOV   R0, #101         ; 'e'
        STORE R0, [0x0002]
        MOV   R0, #108         ; 'l'
        STORE R0, [0x0004]
        MOV   R0, #108         ; 'l'
        STORE R0, [0x0006]
        MOV   R0, #111         ; 'o'
        STORE R0, [0x0008]
        MOV   R0, #44          ; ','
        STORE R0, [0x000A]
        MOV   R0, #32          ; ' '
        STORE R0, [0x000C]
        MOV   R0, #83          ; 'S'
        STORE R0, [0x000E]
        MOV   R0, #111         ; 'o'
        STORE R0, [0x0010]
        MOV   R0, #102         ; 'f'
        STORE R0, [0x0012]
        MOV   R0, #116         ; 't'
        STORE R0, [0x0014]
        MOV   R0, #67          ; 'C'
        STORE R0, [0x0016]
        MOV   R0, #80          ; 'P'
        STORE R0, [0x0018]
        MOV   R0, #85          ; 'U'
        STORE R0, [0x001A]
        MOV   R0, #33          ; '!'
        STORE R0, [0x001C]
        MOV   R0, #10          ; '\n'
        STORE R0, [0x001E]
        MOV   R0, #0           ; null terminator
        STORE R0, [0x0020]

; ── Set pointer R1 to start of string ───────────────────────
        MOV   R1, #0           ; R1 = 0x0000 (start of data segment)

; ── Print loop ───────────────────────────────────────────────
print_loop:
        LOAD  R0, [R1]         ; R0 = Memory[R1]  (load current char)
        CMP   R0, #0           ; is it null terminator?
        JZ    done             ; yes — stop

        OUT   [0xF002], R0     ; write char to MMIO STDOUT
        INC   R1               ; advance pointer by 1
        INC   R1               ; (each word is 2 bytes, so +2)
        JMP   print_loop       ; next character

done:
        HALT

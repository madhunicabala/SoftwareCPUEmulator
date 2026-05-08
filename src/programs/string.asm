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
        HALT

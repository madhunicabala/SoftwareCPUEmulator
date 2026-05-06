; fibonacci.asm
; Iterative Fibonacci: stores first 10 values to data segment [0x0000..0x0012]
; R0 = a (current), R1 = b (next), R2 = counter, R3 = data pointer

        MOV   R0, #0          ; a = F(0) = 0
        MOV   R1, #1          ; b = F(1) = 1
        MOV   R2, #10         ; loop 10 times
        MOV   R3, #0          ; data pointer = 0x0000

loop:
        STORE R0, [R3]        ; mem[R3] = a
        ADD   R3, #2          ; pointer += 2 (next word)
        PUSH  R1              ; save b
        ADD   R1, R0          ; b = a + b  (new b)
        POP   R0              ; a = old b  (new a)
        DEC   R2              ; counter--
        CMP   R2, #0          ; test counter
        JNZ   loop            ; if counter != 0, loop

        HALT

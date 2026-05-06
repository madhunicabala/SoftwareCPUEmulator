; factorial.asm
; Recursive factorial: compute 5! = 120
; Convention: R0 = argument on entry, return value on exit

        MOV   R0, #5          ; n = 5
        CALL  factorial
        HALT                  ; R0 = 120

factorial:
        CMP   R0, #1          ; compare n to 1
        JZ    base_case       ; if n == 1, return 1
        JL    base_case       ; if n <  1 (n == 0), return 1
        PUSH  R0              ; save n on stack
        DEC   R0              ; n = n - 1
        CALL  factorial       ; R0 = (n-1)!
        POP   R1              ; R1 = saved n
        MUL   R0, R1          ; R0 = (n-1)! * n = n!
        RET

base_case:
        MOV   R0, #1          ; return 1
        RET

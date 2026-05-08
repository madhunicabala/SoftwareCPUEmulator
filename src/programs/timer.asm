; timer.asm — MMIO timer demonstration
;
; Shows the Fetch / Compute / Store cycle explicitly:
;   FETCH  — IN  R0, [0xF000]  reads the timer tick count from MMIO
;   COMPUTE — CMP R0, #10      computes (R0 - 10) and sets flags
;   STORE  — STORE R0, [0x0000] writes the sampled count to data segment
;
; MMIO ports used:
;   0xF000  TIMER_COUNT — read current tick count (increments each cpu_step)
;   0xF001  TIMER_CTRL  — write 1 to start, write 0 to stop and reset
;
; After the loop the data segment [0x0000..0x0013] holds 10 sampled
; tick values (one 16-bit word per sample).  Run with:
;   ./bin/emu16 dump build/timer.bin
; to see the samples in the memory dump.

        ; ── Start the hardware timer ─────────────────────
        MOV   R0, #1           ; control value = 1 (start)
        OUT   R0, [0xF001]     ; MMIO[TIMER_CTRL] = 1  → timer starts

        ; ── Initialise loop registers ────────────────────
        MOV   R1, #0           ; R1 = data-segment pointer (0x0000)
        MOV   R2, #10          ; R2 = sample counter (collect 10 values)

        ; ── Polling loop: sample timer every iteration ───
        ;
        ; Each iteration is one complete Fetch / Compute / Store cycle:
        ;
        ;   FETCH   IN  R0, [0xF000]    — read current timer tick
        ;   COMPUTE CMP R0, [threshold] — check if we have enough samples
        ;   STORE   STORE R0, [R1]      — write sample to data segment
        ;
poll:
        IN    R0, [0xF000]     ; FETCH  — read timer tick count into R0
        CMP   R2, #0           ; COMPUTE — are we done collecting?
        JZ    done
        STORE R0, [R1]         ; STORE  — save sample to data segment
        ADD   R1, #2           ; advance data pointer (16-bit words)
        DEC   R2               ; one fewer sample needed
        JMP   poll

        ; ── Stop the timer ───────────────────────────────
done:
        MOV   R0, #0
        OUT   R0, [0xF001]     ; MMIO[TIMER_CTRL] = 0  → timer stops + resets

        HALT

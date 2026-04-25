# SoftCPU-C Memory Map

Total address space: 64KB (0x0000 – 0xFFFF)

```
0x0000 ┌─────────────────────┐
       │   Data Segment      │  4 KB — global variables, strings
0x0FFF └─────────────────────┘
0x1000 ┌─────────────────────┐
       │   Stack Segment     │  4 KB — grows downward from 0x1FFF
       │   SP starts here →  │         SP = 0x1FFF on reset
0x1FFF └─────────────────────┘
0x2000 ┌─────────────────────┐
       │   Code Segment      │  52 KB — program instructions
       │   PC starts here →  │          PC = 0x2000 on reset
0xEFFF └─────────────────────┘
0xF000 ┌─────────────────────┐
       │   MMIO              │  256 B — timer, stdout, stdin
0xF0FF └─────────────────────┘
0xF100 ┌─────────────────────┐
       │   Reserved          │
0xFFFF └─────────────────────┘
```

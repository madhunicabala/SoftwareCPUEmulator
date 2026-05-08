CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g -Iinclude

CPU_TARGET  = bin/emu16
ASM_TARGET  = bin/emu16asm

CPU_SRCS = src/main.c \
           src/emulator/cpu.c \
           src/emulator/memory.c \
           src/emulator/alu.c

ASM_SRCS = src/assembler/assembler.c \
           src/assembler/lexer.c

CPU_OBJS = $(CPU_SRCS:.c=.o)
ASM_OBJS = $(ASM_SRCS:.c=.o)

all: bin $(CPU_TARGET) $(ASM_TARGET)

bin:
	mkdir -p bin

$(CPU_TARGET): $(CPU_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(ASM_TARGET): $(ASM_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(CPU_OBJS) $(ASM_OBJS) $(CPU_TARGET) $(ASM_TARGET)
	rm -rf bin

.PHONY: all clean

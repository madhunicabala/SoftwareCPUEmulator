CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g -Iinclude

CPU_TARGET  = bin/emu16
ASM_TARGET  = bin/emu16asm

CPU_SRCS = src/main.c \
           src/emulator/cpu.c \
           src/emulator/memory.c \
           src/emulator/alu.c \
           src/emulator/trace.c

ASM_SRCS = src/assembler/assembler.c \
           src/assembler/lexer.c

CPU_OBJS = $(CPU_SRCS:.c=.o)
ASM_OBJS = $(ASM_SRCS:.c=.o)

TEST_BINS = bin/test_alu bin/test_cpu bin/test_memory bin/test_assembler

all: bin $(CPU_TARGET) $(ASM_TARGET)

bin:
	mkdir -p bin

$(CPU_TARGET): $(CPU_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(ASM_TARGET): $(ASM_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# ── Unit tests ──────────────────────────────────────────────
bin/test_alu: tests/test_alu.c src/emulator/alu.o | bin
	$(CC) $(CFLAGS) -o $@ $^

bin/test_cpu: tests/test_cpu.c src/emulator/cpu.o src/emulator/memory.o src/emulator/alu.o | bin
	$(CC) $(CFLAGS) -o $@ $^

bin/test_memory: tests/test_memory.c src/emulator/memory.o | bin
	$(CC) $(CFLAGS) -o $@ $^

bin/test_assembler: tests/test_assembler.c src/assembler/lexer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

test: $(TEST_BINS)
	@echo ""; \
	status=0; \
	for t in $(TEST_BINS); do \
	    echo "Running $$t ..."; ./$$t || status=1; echo ""; \
	done; \
	if [ $$status -eq 0 ]; then echo "All tests passed."; \
	else echo "One or more tests FAILED."; fi; \
	exit $$status

clean:
	rm -f $(CPU_OBJS) $(ASM_OBJS) $(CPU_TARGET) $(ASM_TARGET) $(TEST_BINS)
	rm -rf bin

.PHONY: all test clean

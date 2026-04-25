CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g -Iinclude
TARGET  = bin/softcpu
SRCS    = src/main.c \
          src/emulator/cpu.c \
          src/emulator/memory.c \
          src/emulator/alu.c
OBJS    = $(SRCS:.c=.o)

all: bin $(TARGET)

bin:
	mkdir -p bin

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf bin

.PHONY: all clean

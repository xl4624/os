include ../config.mk

CFLAGS  := -ffreestanding -O2 -g -Wall -Wextra -nostdlib -mgeneral-regs-only
LDFLAGS := -T link.ld -nostdlib

PROGS := hello.elf

.PHONY: all clean

all: $(PROGS)

hello.elf: crt0.o hello.o link.ld
	@$(CC) $(LDFLAGS) -o $@ crt0.o hello.o -lgcc

crt0.o: crt0.S
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c syscall.h
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f *.o $(PROGS)

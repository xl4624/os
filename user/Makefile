include ../config.mk

CFLAGS  := -ffreestanding -O2 -g -Wall -Wextra -nostdlib -mgeneral-regs-only
LDFLAGS := -T link.ld -nostdlib

PROGS := $(BUILDDIR)/user/hello.elf

.PHONY: all clean

all: $(PROGS)

$(BUILDDIR)/user/hello.elf: $(BUILDDIR)/user/crt0.o $(BUILDDIR)/user/hello.o link.ld
	@mkdir -p $(dir $@)
	@$(CC) $(LDFLAGS) -o $@ $(BUILDDIR)/user/crt0.o $(BUILDDIR)/user/hello.o -lgcc

$(BUILDDIR)/user/crt0.o: crt0.S
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/user/%.o: %.c syscall.h
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(BUILDDIR)/user/*.o $(PROGS)

include ../config.mk

CFLAGS  := --sysroot=$(SYSROOT) -isystem $(SYSROOT)/usr/include \
           -ffreestanding -O2 -g -Wall -Wextra -nostdlib -mgeneral-regs-only
LDFLAGS := -T link.ld -nostdlib --sysroot=$(SYSROOT)
LIBS    := -L$(SYSROOT)/usr/lib -lc -lgcc

PROGS := $(BUILDDIR)/user/hello.elf

.PHONY: all clean

all: $(PROGS)

$(BUILDDIR)/user/hello.elf: $(BUILDDIR)/user/crt0.o $(BUILDDIR)/user/hello.o link.ld
	@mkdir -p $(dir $@)
	@$(CC) $(LDFLAGS) -o $@ $(BUILDDIR)/user/crt0.o $(BUILDDIR)/user/hello.o $(LIBS)

$(BUILDDIR)/user/crt0.o: crt0.S
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/user/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(BUILDDIR)/user/*.o $(PROGS)

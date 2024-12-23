# ==== Toolchain Settings ====
CROSS    := i686-elf
CC       := $(CROSS)-gcc
CPP      := $(CROSS)-g++
AS       := $(CROSS)-as
LD       := $(CROSS)-ld
AR       := $(CROSS)-ar
GRUBISO  := grub-mkrescue
GRUBFILE := grub-file

# ==== Build Settings ====
CFLAGS  := -ffreestanding -O2 -g -Wall -Wextra
LDFLAGS := -T arch/linker.ld -ffreestanding -O2 -nostdlib
LIBS    := -nostdlib -Lkernel -lkernel -Llibc -lk -lgcc
SUBDIRS := arch kernel libc

# ==== Runtime Files ====
CRTI	 := arch/crti.o
CRTN	 := arch/crtn.o
CRTBEGIN := $(shell $(CPP) -print-file-name=crtbegin.o)
CRTEND	 := $(shell $(CPP) -print-file-name=crtend.o)

# ==== Build Artifacts ====
ARCH_OBJS  := $(wildcard arch/*.o)
KERNEL_LIB := kernel/libkernel.a
LIBC_OBJS  := $(wildcard libc/*.libk.o)
OBJS       := $(CRTI) $(CRTBEGIN) arch/boot.o $(LIBC_OBJS) $(CRTEND) $(CRTN)

# ==== Targets ====
BIN     := myos.bin
ISO     := myos.iso
ISODIR  := isodir
GRUBCFG := grub.cfg

.PHONY: all clean build iso check run arch kernel libc

all: $(ISO)

run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f $(BIN) $(ISO)
	rm -rf $(ISODIR)

check: $(BIN)
	@if $(GRUBFILE) --is-x86-multiboot $(BIN); then \
		echo "multiboot confirmed"; \
	else \
		echo "NOT MULTIBOOT"; \
	fi

$(BIN): build $(OBJS) $(KERNEL_LIB) arch/linker.ld
	$(CC) $(LDFLAGS) -o $(BIN) $(OBJS) $(LIBS)

$(ISO): $(BIN)
	mkdir -p $(ISODIR)/boot/grub
	cp $(BIN) $(ISODIR)/boot/$(BIN)
	cp $(GRUBCFG) $(ISODIR)/boot/grub/grub.cfg
	$(GRUBISO) -o $(ISO) $(ISODIR)

build: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

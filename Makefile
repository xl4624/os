include config.mk

export SYSROOT := $(CURDIR)/sysroot

# ==== Build Settings ====
LDFLAGS := -T arch/linker.ld -ffreestanding -O2 -nostdlib
LIBS    := -nostdlib -Llibc -lk -lgcc
SUBDIRS := arch kernel libc

# ==== Runtime Files ====
CRTI	 := arch/crti.o
CRTN	 := arch/crtn.o
CRTBEGIN := $(shell $(CPP) -print-file-name=crtbegin.o)
CRTEND	 := $(shell $(CPP) -print-file-name=crtend.o)

# ==== Build Artifacts ====
KERNEL_OBJS = $(shell find kernel -type f -name '*.o')
LIBC_OBJS   = $(shell find libc -type f -name '*.libk.o')
OBJS        = $(CRTI) $(CRTBEGIN) arch/boot.o $(KERNEL_OBJS) $(LIBC_OBJS) $(CRTEND) $(CRTN)

# ==== Targets ====
BIN := myos.bin
ISODIR = isodir
ISO = myos.iso

.PHONY: all run debug clean check install arch kernel libc

all: $(BIN)

run: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -monitor stdio

debug: $(ISO)
	qemu-system-i386 -s -S -cdrom $(ISO) -monitor stdio

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f $(BIN) $(ISO)
	rm -rf $(ISODIR) $(SYSROOT)

install:
	python install.py

$(BIN): $(SUBDIRS) $(OBJS) arch/linker.ld
	$(CC) $(LDFLAGS) -o $(BIN) $(OBJS) $(LIBS)
	@grub-file --is-x86-multiboot $@ || { echo "NOT MULTIBOOT"; exit 1; }

$(ISO): $(BIN)
	mkdir -p $(ISODIR)/boot/grub
	cp $< $(ISODIR)/boot/
	cp grub.cfg $(ISODIR)/boot/grub/
	grub-mkrescue -o $@ $(ISODIR)

$(SUBDIRS): install
	$(MAKE) -C $@

print-sysroot:
	@echo $(SYSROOT)

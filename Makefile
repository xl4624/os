include config.mk

export SYSROOT := $(CURDIR)/sysroot

# ==== Build Settings ====
LDFLAGS := -T arch/linker.ld -ffreestanding -O2 -nostdlib
LIBS    := -nostdlib -Llibc -lk -lgcc
SUBDIRS := arch kernel libc

# Disable built-in rules for kernel test files (they're built by kernel/Makefile)
kernel/test/%.o: kernel/test/%.cpp
	@true

# ==== Runtime Files ====
CRTI	 := arch/crti.o
CRTN	 := arch/crtn.o
CRTBEGIN := $(shell $(CPP) -print-file-name=crtbegin.o)
CRTEND	 := $(shell $(CPP) -print-file-name=crtend.o)

# ==== Build Artifacts ====
KERNEL_OBJS = $(shell find kernel -type f -name '*.o')
LIBC_OBJS   = $(shell find libc -type f -name '*.libk.o')
OBJS        = $(CRTI) $(CRTBEGIN) arch/boot.o $(KERNEL_OBJS) $(LIBC_OBJS) $(CRTEND) $(CRTN)

# Test build artifacts (rebuild kernel with tests enabled)
# Note: KTEST_KERNEL_OBJS is expanded at recipe time, not parse time

# ==== Targets ====
BIN       := myos.bin
ISODIR     = isodir
ISO        = myos.iso
KTEST_BIN := myos-test.bin
KTEST_ISO := myos-test.iso

.PHONY: all run debug clean check install arch kernel libc test clean-test ktest ktest-kernel

all: $(BIN)

run: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -no-reboot -no-shutdown

debug: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -s -S -monitor stdio -no-reboot -no-shutdown

test:
	$(MAKE) -C tests unit

# Run kernel tests in QEMU
ktest: install $(KTEST_ISO)
	@echo "Running kernel tests in QEMU..."
	qemu-system-i386 -no-reboot -cdrom $(KTEST_ISO) -device isa-debug-exit,iobase=0xf4,iosize=0x04; true
	$(MAKE) -C kernel clean KERNEL_TESTS=1

clean: clean-test
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f $(BIN) $(ISO) $(KTEST_BIN) $(KTEST_ISO)
	rm -rf $(ISODIR) $(SYSROOT)

clean-test:
	$(MAKE) -C tests clean

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

# Build kernel objects with tests enabled
ktest-kernel:
	$(MAKE) -C kernel clean
	$(MAKE) -C kernel KERNEL_TESTS=1

# Link the test kernel binary
$(KTEST_BIN): arch libc ktest-kernel arch/linker.ld
	$(CC) $(LDFLAGS) -o $@ \
		$(CRTI) $(CRTBEGIN) arch/boot.o \
		$$(find kernel -type f -name '*.o' | sort) \
		$$(find libc   -type f -name '*.libk.o' | sort) \
		$(CRTEND) $(CRTN) $(LIBS)
	@grub-file --is-x86-multiboot $@ || { echo "NOT MULTIBOOT"; exit 1; }

$(KTEST_ISO): $(KTEST_BIN)
	mkdir -p $(ISODIR)/boot/grub
	cp $< $(ISODIR)/boot/myos.bin
	cp grub.cfg $(ISODIR)/boot/grub/
	grub-mkrescue -o $@ $(ISODIR)


$(SUBDIRS): install
	$(MAKE) -C $@

print-sysroot:
	@echo $(SYSROOT)

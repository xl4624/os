include config.mk

export SYSROOT := $(CURDIR)/sysroot

# ==== Build Settings ====
LDFLAGS := -T arch/linker.ld -ffreestanding -O2 -nostdlib
LIBS    := -nostdlib -Llibc -lk -lgcc
SUBDIRS := arch kernel libc user

# Disable built-in rules for kernel test files (they're built by kernel/Makefile)
kernel/test/%.o: kernel/test/%.cpp
	@true

# ==== Runtime Files ====
CRTI	 := arch/crti.o
CRTN	 := arch/crtn.o
CRTBEGIN := $(shell $(CPP) -print-file-name=crtbegin.o)
CRTEND	 := $(shell $(CPP) -print-file-name=crtend.o)

# ==== Build Artifacts ====
KERNEL_OBJS = $(shell find kernel -type f -name '*.o' ! -path 'kernel/test/*')
LIBC_OBJS   = $(shell find libc -type f -name '*.libk.o')
OBJS        = $(CRTI) $(CRTBEGIN) arch/boot.o $(KERNEL_OBJS) $(LIBC_OBJS) $(CRTEND) $(CRTN)

# ==== Targets ====
BIN       := myos.bin
ISODIR     = isodir
ISO        = myos.iso
KTEST_BIN := myos-test.bin
KTEST_ISO := myos-test.iso

.PHONY: all run debug lldb clean check install arch kernel libc user test ktest ktest-kernel

all: $(BIN)

run: $(ISO)
	@qemu-system-i386 -cdrom $(ISO) -no-reboot -no-shutdown

debug: $(ISO)
	@qemu-system-i386 -cdrom $(ISO) -s -S -monitor stdio -no-reboot -no-shutdown

lldb: $(ISO) $(BIN)
	@qemu-system-i386 -cdrom $(ISO) -s -S -no-reboot -no-shutdown &
	@sleep 0.5
	@lldb $(BIN) \
		-o "protocol-server start MCP listen://localhost:59999" \
		-o "gdb-remote localhost:1234"

test:
	@$(MAKE) -C tests unit

ktest: install $(KTEST_ISO)
	@echo "Running kernel tests in QEMU..."
	@qemu-system-i386 -no-reboot -cdrom $(KTEST_ISO) -device isa-debug-exit,iobase=0xf4,iosize=0x04; \
		status=$$?; \
		if [ $$status -eq 1 ]; then \
			echo "[SUCCESS] All tests passed!"; \
			result=0; \
		elif [ $$status -eq 3 ]; then \
			echo "[FAILURE] Tests failed (exit code: $$status)"; \
			result=1; \
		elif [ $$status -eq 0 ]; then \
			echo "[FAILURE] QEMU exited unexpectedly (exit code: $$status)"; \
			result=1; \ \
		else \
			echo "[FAILURE] Unknown error (exit code: $$status)"; \
			result=$$status; \
		fi; \
		$(MAKE) -C kernel clean; \
		exit $$result

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	@$(MAKE) -C tests clean
	@rm -f $(BIN) $(ISO) $(KTEST_BIN) $(KTEST_ISO)
	@rm -rf $(ISODIR) $(SYSROOT)

install:
	@./install.sh

$(BIN): $(SUBDIRS) $(OBJS) arch/linker.ld
	@$(CC) $(LDFLAGS) -o $(BIN) $(OBJS) $(LIBS)
	@grub-file --is-x86-multiboot $@ || { echo "NOT MULTIBOOT"; exit 1; }

$(ISO): $(BIN) user
	@mkdir -p $(ISODIR)/boot/grub
	@cp $< $(ISODIR)/boot/
	@cp user/hello.elf $(ISODIR)/boot/
	@cp grub.cfg $(ISODIR)/boot/grub/
	@grub-mkrescue -o $@ $(ISODIR)

# Build kernel objects with tests enabled
ktest-kernel:
	@$(MAKE) -C kernel clean
	@$(MAKE) -C kernel KERNEL_TESTS=1

# Link the test kernel binary
$(KTEST_BIN): install arch libc ktest-kernel arch/linker.ld
	@$(CC) $(LDFLAGS) -o $@ \
		$(CRTI) $(CRTBEGIN) arch/boot.o \
		$$(find kernel -type f -name '*.o' | sort) \
		$$(find libc   -type f -name '*.libk.o' | sort) \
		$(CRTEND) $(CRTN) $(LIBS)
	@grub-file --is-x86-multiboot $@ || { echo "NOT MULTIBOOT"; exit 1; }

$(KTEST_ISO): $(KTEST_BIN)
	@mkdir -p $(ISODIR)/boot/grub
	@cp $< $(ISODIR)/boot/myos.bin
	@cp grub.cfg $(ISODIR)/boot/grub/
	@grub-mkrescue -o $@ $(ISODIR)

arch kernel libc: install
	@$(MAKE) -C $@

user:
	@$(MAKE) -C $@

print-sysroot:
	@echo $(SYSROOT)

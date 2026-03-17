include config.mk

export SYSROOT  := $(CURDIR)/sysroot
export BUILDDIR := $(CURDIR)/build

KTEST_BUILDDIR := $(CURDIR)/build/ktest

# ==== Build Settings ====
LDFLAGS := -T arch/linker.ld -ffreestanding -O2 -nostdlib
LIBS    := -nostdlib -L$(BUILDDIR)/libc -lk -lgcc
SUBDIRS := arch libc libcpp kernel user

# ==== Runtime Files ====
CRTI	 := $(BUILDDIR)/arch/crti.o
CRTN	 := $(BUILDDIR)/arch/crtn.o
CRTBEGIN := $(shell $(CPP) -print-file-name=crtbegin.o)
CRTEND	 := $(shell $(CPP) -print-file-name=crtend.o)

DISK_IMG := $(BUILDDIR)/disk.img

# ==== Build Artifacts ====
KERNEL_OBJS = $(shell find $(BUILDDIR)/kernel -type f -name '*.o' 2>/dev/null)
LIBC_OBJS   = $(shell find $(BUILDDIR)/libc -type f -name '*.libk.o' 2>/dev/null)
LIBCPP_OBJS = $(shell find $(BUILDDIR)/libcpp -type f -name '*.o' 2>/dev/null)
OBJS        = $(CRTI) $(CRTBEGIN) $(BUILDDIR)/arch/boot.o $(KERNEL_OBJS) $(LIBCPP_OBJS) $(LIBC_OBJS) $(CRTEND) $(CRTN)

# ==== Targets ====
BIN       := $(BUILDDIR)/myos.bin
ISODIR     = $(BUILDDIR)/isodir
ISO        = $(BUILDDIR)/myos.iso
KTEST_BIN := $(KTEST_BUILDDIR)/myos-test.bin
KTEST_ISO := $(KTEST_BUILDDIR)/myos-test.iso

.PHONY: all run debug lldb clean check install arch kernel libc libcpp user test ktest disk

all: $(ISO)

RUN_LOG := $(BUILDDIR)/run.log

run: $(ISO) $(DISK_IMG)
	@qemu-system-i386 -m 256 -cdrom $(ISO) -boot order=d -no-reboot \
		-drive file=$(DISK_IMG),format=raw,if=ide,index=0 \
		-device isa-debug-exit,iobase=0xf4,iosize=0x04 \
		-display sdl,grab-mod=lshift-lctrl-lalt -debugcon file:$(RUN_LOG); true
	@echo "--- Log written to $(RUN_LOG) ---"

debug: $(ISO) $(DISK_IMG)
	@qemu-system-i386 -m 256 -cdrom $(ISO) -boot order=d \
		-drive file=$(DISK_IMG),format=raw,if=ide,index=0 \
		-s -S -monitor stdio -no-reboot -no-shutdown

lldb: $(ISO) $(BIN) $(DISK_IMG)
	@qemu-system-i386 -m 256 -cdrom $(ISO) -boot order=d \
		-drive file=$(DISK_IMG),format=raw,if=ide,index=0 \
		-s -S -no-reboot -no-shutdown &
	@sleep 0.5
	@lldb $(BIN) \
		-o "protocol-server start MCP listen://localhost:59999" \
		-o "gdb-remote localhost:1234"

test: libc
	@$(MAKE) -C tests unit

KTEST_LOG := $(KTEST_BUILDDIR)/ktest.log

ktest: install $(KTEST_ISO)
	@echo "Running kernel tests in QEMU..."
	@qemu-system-i386 -m 256 -no-reboot -display none -cdrom $(KTEST_ISO) \
		-device isa-debug-exit,iobase=0xf4,iosize=0x04 \
		-debugcon file:$(KTEST_LOG); \
		status=$$?; \
		if [ $$status -eq 1 ]; then \
			echo "[SUCCESS] All tests passed!"; \
			result=0; \
		elif [ $$status -eq 3 ]; then \
			echo "[FAILURE] Tests failed (exit code: $$status)"; \
			result=1; \
		elif [ $$status -eq 0 ]; then \
			echo "[FAILURE] QEMU exited unexpectedly (exit code: $$status)"; \
			result=1; \
		else \
			echo "[FAILURE] Unknown error (exit code: $$status)"; \
			result=$$status; \
		fi; \
		cat $(KTEST_LOG); \
		exit $$result

disk: $(DISK_IMG)

# Create a 32 MB FAT16 disk image.
$(DISK_IMG): | $(BUILDDIR)
	@dd if=/dev/zero of=$@ bs=512 count=65536 status=none
	@mkfs.fat -F 16 -n "MYOS" $@ > /dev/null
	@printf "Hello from FAT16 on MYOS!\n" | mcopy -i $@ - ::readme.txt

clean:
	@rm -rf $(BUILDDIR) $(SYSROOT)

install:
	@./install.sh

$(BIN): $(SUBDIRS) arch/linker.ld
	@$(CC) $(LDFLAGS) -o $(BIN) $(OBJS) $(LIBS)
	@grub-file --is-x86-multiboot $@ || { echo "NOT MULTIBOOT"; exit 1; }

$(ISO): $(BIN) user
	@mkdir -p $(ISODIR)/boot/grub
	@cp $(BIN) $(ISODIR)/boot/myos.bin
	@for f in $(BUILDDIR)/user/*.elf; do cp "$$f" $(ISODIR)/boot/; done
	@if ls user/doom/*.wad 2>/dev/null | grep -q .; then \
		for f in user/doom/*.wad; do cp "$$f" $(ISODIR)/boot/; done; \
	fi
	@cp grub.cfg $(ISODIR)/boot/grub/
	@grub-mkrescue -o $@ $(ISODIR)

# Build kernel objects with tests enabled and link the test binary
$(KTEST_BIN): install arch libc libcpp arch/linker.ld
	@$(MAKE) -C kernel BUILDDIR=$(KTEST_BUILDDIR) KERNEL_TESTS=1
	@mkdir -p $(dir $@)
	@$(CC) $(LDFLAGS) -o $@ \
		$(CRTI) $(CRTBEGIN) $(BUILDDIR)/arch/boot.o \
		$$(find $(KTEST_BUILDDIR)/kernel -type f -name '*.o' | sort) \
		$$(find $(BUILDDIR)/libcpp -type f -name '*.o' | sort) \
		$$(find $(BUILDDIR)/libc -type f -name '*.libk.o' | sort) \
		$(CRTEND) $(CRTN) $(LIBS)
	@grub-file --is-x86-multiboot $@ || { echo "NOT MULTIBOOT"; exit 1; }

$(KTEST_ISO): $(KTEST_BIN)
	@mkdir -p $(KTEST_BUILDDIR)/isodir/boot/grub
	@cp $< $(KTEST_BUILDDIR)/isodir/boot/myos.bin
	@cp grub-test.cfg $(KTEST_BUILDDIR)/isodir/boot/grub/grub.cfg
	@grub-mkrescue -o $@ $(KTEST_BUILDDIR)/isodir

$(BUILDDIR):
	@mkdir -p $@

arch kernel libc libcpp: install
	@$(MAKE) -C $@

user: install libc
	@$(MAKE) -C libc SYSROOT=$(SYSROOT) install
	@$(MAKE) -C $@

print-sysroot:
	@echo $(SYSROOT)

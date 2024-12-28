# ==== Toolchain Settings ====
CROSS    := i686-elf
CC       := $(CROSS)-gcc
CPP      := $(CROSS)-g++
AS       := $(CROSS)-as
LD       := $(CROSS)-ld
AR       := $(CROSS)-ar
GRUBFILE := grub-file

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
BIN     := myos.bin

.PHONY: all clean build check arch kernel libc

all: $(BIN)

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f $(BIN)

check: $(BIN)
	@if $(GRUBFILE) --is-x86-multiboot $(BIN); then \
		echo "multiboot confirmed"; \
	else \
		echo "NOT MULTIBOOT"; \
	fi

$(BIN): $(SUBDIRS) $(OBJS) arch/linker.ld
	$(CC) $(LDFLAGS) -o $(BIN) $(OBJS) $(LIBS)

$(SUBDIRS):
	$(MAKE) -C $@

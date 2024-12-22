CROSS := i686-elf
AS    := $(CROSS)-as
CC    := $(CROSS)-gcc
CXX   := $(CROSS)-g++

CCFLAGS  := -std=gnu99 -ffreestanding -O2 -Wall -Wextra
CXXFLAGS := -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti
LDFLAGS  := -T arch/linker.ld -ffreestanding -O2 -nostdlib

CRTI	 := build/crti.o
CRTN	 := build/crtn.o
CRTBEGIN := $(shell $(CC) -print-file-name=crtbegin.o)
CRTEND	 := $(shell $(CC) -print-file-name=crtend.o)

GRUBFILE := grub-file
GRUBISO  := grub-mkrescue

KERNEL_SRCS := $(wildcard kernel/*.cpp)
KERNEL_OBJS := $(KERNEL_SRCS:%.cpp=build/%.o)

BIN := myos.bin
ISO := myos.iso

.PHONY: all clean check iso
all: iso

$(BIN): $(CRTI) $(CRTBEGIN) $(KERNEL_OBJS) build/boot.o $(CRTEND) $(CRTN)
	$(CC) $(LDFLAGS) $^ -o $@ -lgcc

build/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/boot.o: arch/boot.s
	mkdir -p $(dir $@)
	$(AS) $< -o $@

$(CRTI): arch/crti.s
	mkdir -p $(dir $@)
	$(AS) $< -o $@

$(CRTN): arch/crtn.s
	mkdir -p $(dir $@)
	$(AS) $< -o $@

check: $(BIN)
	@if $(GRUBFILE) --is-x86-multiboot $(BIN); then \
		echo "multiboot confirmed"; \
	else \
		echo "NOT MULTIBOOT"; \
	fi

iso: $(BIN)
	mkdir -p isodir/boot/grub
	cp $(BIN) isodir/boot/$(BIN)
	cp grub.cfg isodir/boot/grub/grub.cfg
	$(GRUBISO) -o $(ISO) isodir

clean:
	rm -f $(KERNEL_OBJS) $(BIN) $(ISO) $(CRTI) $(CRTN)
	rm -rf isodir/ build/

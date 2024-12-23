# ==== Toolchain Settings ====
CROSS ?= i686-elf
AS    := $(CROSS)-as

# ==== Source and Object Files ====
SRCS    := $(wildcard *.s)
OBJS    := $(SRCS:%.s=%.o)

.PHONY: all clean

all: $(OBJS) linker.ld

%.o: %.s
	$(AS) $< -o $@

clean:
	rm -f $(OBJS)

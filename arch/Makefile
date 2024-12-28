include ../config.mk

# ==== Source and Object Files ====
SRCS    := $(wildcard *.S)
OBJS    := $(SRCS:%.S=%.o)

.PHONY: all clean

all: $(OBJS) linker.ld

%.o: %.S
	$(AS) $< -o $@

clean:
	rm -f $(OBJS)

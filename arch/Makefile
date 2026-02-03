include ../config.mk

# ==== Source and Object Files ====
SRCS    := $(wildcard *.S)
OBJS    := $(patsubst %.S,$(BUILDDIR)/arch/%.o,$(SRCS))

.PHONY: all clean

all: $(OBJS) linker.ld

$(BUILDDIR)/arch/%.o: %.S
	@mkdir -p $(dir $@)
	$(AS) $< -o $@

clean:
	rm -f $(OBJS)

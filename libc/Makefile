include ../config.mk

CFLAGS   := ${BASE_CFLAGS} -Iinclude
CFLAGS_K := $(CFLAGS) -D__is_libk
CFLAGS_C := $(CFLAGS) -D__is_libc

# ==== Source and Object Files ====
SRCS      := $(shell find . -type f -name "*.c")
KOBJS     := $(patsubst ./%.c,$(BUILDDIR)/libc/%.libk.o,$(SRCS))
COBJS     := $(patsubst ./%.c,$(BUILDDIR)/libc/%.libc.o,$(SRCS))
TARGET_K  := $(BUILDDIR)/libc/libk.a
TARGET_C  := $(BUILDDIR)/libc/libc.a

.PHONY: all clean install install-headers install-libs

all: $(TARGET_K) $(TARGET_C)

$(BUILDDIR)/libc/%.libk.o: ./%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_K) $(INCLUDES) -c $< -o $@

$(TARGET_K): $(KOBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(BUILDDIR)/libc/%.libc.o: ./%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_C) $(INCLUDES) -c $< -o $@

$(TARGET_C): $(COBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

DEPS := $(KOBJS:.o=.d) $(COBJS:.o=.d)
-include $(DEPS)

clean:
	rm -f $(KOBJS) $(COBJS) $(TARGET_K) $(TARGET_C) $(DEPS)

install: install-headers install-libs

install-headers:
	mkdir -p $(SYSROOT)$(INCLUDEDIR)
	cp -R -p include/. $(SYSROOT)$(INCLUDEDIR)/.

install-libs: $(TARGET_K) $(TARGET_C)
	mkdir -p $(SYSROOT)$(LIBDIR)
	cp $(TARGET_K) $(TARGET_C) $(SYSROOT)$(LIBDIR)

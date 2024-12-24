#=== Toolchain Settings ====
CROSS ?= i686-elf
CC    :=$(CROSS)-gcc
AR    := $(CROSS)-ar

# Assume sysroot is in ../sysroot
SYSROOT     := $(CURDIR)/../sysroot
PREFIX      := /usr
INCLUDEDIR  := $(PREFIX)/include
LIBDIR      ?= $(PREFIX)/lib

# ==== Build Settings ====
CFLAGS   := --sysroot=$(SYSROOT) -isystem $(SYSROOT)/usr/include -O2 -g -ffreestanding -Wall -Wextra -Iinclude
CFLAGS_K := $(CFLAGS) -D__is_libk
CFLAGS_C := $(CFLAGS) -D__is_libc

# ==== Source and Object Files ====
SRCS      := $(shell find . -type f -name "*.c")
KOBJS     := $(SRCS:%.c=%.libk.o)
COBJS     := $(SRCS:%.c=%.libc.o)
TARGET_K  := libk.a
TARGET_C  := libc.a

.PHONY: all clean install install-headers install-libs

all: $(TARGET_K) $(TARGET_C)

%.libk.o: %.c
	$(CC) $(CFLAGS_K) $(INCLUDES) -c $< -o $@

$(TARGET_K): $(KOBJS)
	$(AR) rcs $@ $^

%.libc.o: %.c
	$(CC) $(CFLAGS_C) $(INCLUDES) -c $< -o $@

$(TARGET_C): $(COBJS)
	$(AR) rcs $@ $^

clean:
	rm -f $(KOBJS) $(COBJS) $(TARGET_K) $(TARGET_C)

install: install-headers install-libs

install-headers:
	mkdir -p $(SYSROOT)$(INCLUDEDIR)
	cp -R -p include/. $(SYSROOT)$(INCLUDEDIR)/.

install-libs: $(TARGET_K) $(TARGET_C)
	mkdir -p $(SYSROOT)$(LIBDIR)
	cp $(TARGET_K) $(TARGET_C) $(SYSROOT)$(LIBDIR)

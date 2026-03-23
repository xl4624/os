# ==== Toolchain Settings ====
CROSS    ?= i686-elf
CC       := $(CROSS)-gcc
CPP      := $(CROSS)-g++
AS       := $(CROSS)-as
LD       := $(CROSS)-ld
AR       := $(CROSS)-ar

# ==== Directory Structure ====
SYSROOT  ?= $(CURDIR)/sysroot
BUILDDIR ?= $(CURDIR)/build
PREFIX     := /usr
INCLUDEDIR := $(PREFIX)/include
LIBDIR     := $(PREFIX)/lib

# ==== Kernel Build Flags ====
KERNEL_CFLAGS := --sysroot=$(SYSROOT) -isystem $(SYSROOT)/usr/include/c -O2 -g \
                 -ffreestanding -Wall -Wextra -Wpedantic -Wconversion -MMD -MP
KERNEL_CXXFLAGS := $(KERNEL_CFLAGS) -std=gnu++20 -isystem $(SYSROOT)/usr/include/cpp

# ==== Userspace Build Flags ====
LIBC_CFLAGS := $(KERNEL_CFLAGS) -D__is_libc
LIBC_CXXFLAGS := $(KERNEL_CXXFLAGS) -D__is_libc

# ==== Unit Test Flags (host compiler) ====
TEST_CFLAGS := -std=c11 -Wall -Wextra -D__is_libc
TEST_CXXFLAGS := -std=gnu++20 -Wall -Wextra -D__is_libc

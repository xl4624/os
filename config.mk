# ==== Toolchain Settings ====
CROSS    ?= i686-elf
CC       := $(CROSS)-gcc
CPP      := $(CROSS)-g++
AS       := $(CROSS)-as
LD       := $(CROSS)-ld
AR       := $(CROSS)-ar

# ==== Directory Structure ====
PREFIX     := /usr
INCLUDEDIR := $(PREFIX)/include
LIBDIR     := $(PREFIX)/lib

# ==== Common Flags ====
BASE_CFLAGS := --sysroot=$(SYSROOT) -isystem $(SYSROOT)/usr/include -O2 -g \
               -ffreestanding -Wall -Wextra

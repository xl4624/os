# ==== Toolchain Settings ====
CROSS ?= i686-elf
CPP   := $(CROSS)-g++

# Assume sysroot is in ../sysroot
SYSROOT := $(CURDIR)/../sysroot
PREFIX      := /usr
INCLUDEDIR  := $(PREFIX)/include

# ==== Build Settings ====
CFLAGS := --sysroot=$(SYSROOT) -isystem $(SYSROOT)/usr/include -O2 -g -ffreestanding -Wall -Wextra
CPPFLAGS := -fno-exceptions -fno-rtti -fno-threadsafe-statics -D__is_kernel -Iinclude

# ==== Source and Object Files ====
SRCS   := $(wildcard *.cpp)
OBJS   := $(SRCS:%.cpp=%.o)

.PHONY: all clean install

all: $(OBJS)

%.o: %.cpp
	$(CPP) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

install:
	mkdir -p $(SYSROOT)$(INCLUDEDIR)
	cp -R -p include/. $(SYSROOT)$(INCLUDEDIR)/.

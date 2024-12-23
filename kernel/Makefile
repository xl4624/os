# ==== Toolchain Settings ====
CROSS ?= i686-elf
CPP   := $(CROSS)-g++
AR    := $(CROSS)-ar

# Assume sysroot is in ../sysroot
SYSROOT := $(CURDIR)/../sysroot
PREFIX      := /usr
INCLUDEDIR  := $(PREFIX)/include

# ==== Build Settings ====
CFLAGS   := --sysroot=$(SYSROOT) -isystem $(SYSROOT)/usr/include -O2 -g -ffreestanding -Wall -Wextra -fno-exceptions -fno-rtti -fno-threadsafe-statics
CPPFLAGS := -D__is_kernel -Iinclude

# ==== Source and Object Files ====
SRCS   := $(wildcard *.cpp)
OBJS   := $(SRCS:%.cpp=%.o)
TARGET := libkernel.a

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $(OBJS)

%.o: %.cpp
	$(CPP) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install:
	mkdir -p $(SYSROOT)$(INCLUDEDIR)
	cp -R -p include/. $(SYSROOT)$(INCLUDEDIR)/.

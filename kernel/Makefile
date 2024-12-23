# ==== Toolchain Settings ====
CROSS ?= i686-elf
CPP   := $(CROSS)-g++
AR    := $(CROSS)-ar

# Assume sysroot is in ../sysroot
SYSROOT := $(CURDIR)/../sysroot

# ==== Build Settings ====
CFLAGS   := --sysroot=$(SYSROOT) -O2 -g -ffreestanding -Wall -Wextra -fno-exceptions -fno-rtti
CPPFLAGS := -isystem $(SYSROOT)/usr/include -D__is_kernel -Iinclude

# ==== Source and Object Files ====
SRCS   := $(wildcard *.cpp)
OBJS   := $(SRCS:%.cpp=%.o)
TARGET := libkernel.a

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $(OBJS)

%.o: %.cpp
	$(CPP) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

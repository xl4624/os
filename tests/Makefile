include ../config.mk

BUILDDIR ?= $(abspath $(CURDIR)/../build)

# Use host compilers for tests (not the cross compiler)
CXX := g++
CC  := gcc

# All files use myos libc headers
CXXFLAGS := -std=c++17 -Wall -Wextra -D__is_libc -I./framework -I../libcpp/include -I../libc/include
LIBC_CFLAGS := -std=c11 -Wall -Wextra -D__is_libc -I../libc/include

# Test files
UNIT_TESTS := $(wildcard unit/test_*.cpp)
UNIT_OBJS  := $(patsubst unit/%.cpp,$(BUILDDIR)/test/%.o,$(UNIT_TESTS))
MAIN_OBJ   := $(BUILDDIR)/test/main.o

# libc source files that work on host.
# Exclude unistd/ -- those files use int 0x80 syscalls that crash on the host.
LIBC_SRCS := $(shell find ../libc -name '*.c' -not -path '*/unistd/*')
LIBC_OBJS := $(patsubst ../libc/%.c,$(BUILDDIR)/test/libc/%.o,$(LIBC_SRCS))

ALL_OBJS := $(UNIT_OBJS) $(MAIN_OBJ) $(LIBC_OBJS)

# ==== Targets ====
UNIT_BIN := $(BUILDDIR)/test/unit_tests

.PHONY: all unit clean list

all: unit

unit: $(UNIT_BIN)
	$(UNIT_BIN)

$(BUILDDIR)/test/%.o: unit/%.cpp | $(BUILDDIR)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/test/libc/%.o: ../libc/%.c
	mkdir -p $(dir $@)
	$(CC) $(LIBC_CFLAGS) -c $< -o $@

$(UNIT_BIN): $(ALL_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILDDIR)/test:
	mkdir -p $@

clean:
	rm -f $(ALL_OBJS) $(UNIT_BIN)
	rm -rf $(BUILDDIR)/test

list:
	echo "Discovered unit tests:"
	for test in $(UNIT_TESTS); do echo "  $$test"; done

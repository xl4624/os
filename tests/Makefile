include ../config.mk

# Use host compilers for tests (not the cross compiler)
CXX := g++
CC  := gcc

# All files use myos libc headers
CXXFLAGS := -std=c++17 -Wall -Wextra -I./framework -I../libc/include
LIBC_CFLAGS := -std=c11 -Wall -Wextra -I../libc/include

# ==== Source and Object Files ====
UNIT_DIR   := unit
BUILD_DIR  := build

# Test files
UNIT_TESTS := $(wildcard $(UNIT_DIR)/test_*.cpp)
UNIT_OBJS  := $(patsubst $(UNIT_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(UNIT_TESTS))
MAIN_OBJ   := $(BUILD_DIR)/main.o

# libc source files that work on host (exclude kernel-dependent ones)
LIBC_SRCS := $(shell find ../libc/string -name '*.c') \
             ../libc/stdlib/itoa.c
LIBC_OBJS := $(patsubst ../libc/%.c,$(BUILD_DIR)/libc/%.o,$(LIBC_SRCS))

ALL_OBJS := $(UNIT_OBJS) $(MAIN_OBJ) $(LIBC_OBJS)

# ==== Targets ====
UNIT_BIN := $(BUILD_DIR)/unit_tests

.PHONY: all unit clean list

all: unit

unit: $(UNIT_BIN)
	./$(UNIT_BIN)

$(BUILD_DIR)/%.o: $(UNIT_DIR)/%.cpp | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/libc/%.o: ../libc/%.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(LIBC_CFLAGS) -c $< -o $@

$(UNIT_BIN): $(ALL_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -f $(ALL_OBJS) $(UNIT_BIN)
	rm -rf $(BUILD_DIR)

list:
	echo "Discovered unit tests:"
	for test in $(UNIT_TESTS); do echo "  $$test"; done

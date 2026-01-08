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
	@echo "Running unit tests..."
	@./$(UNIT_BIN)

# Build test C++ object files (use system headers)
$(BUILD_DIR)/%.o: $(UNIT_DIR)/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build libc C object files (use myos headers)
$(BUILD_DIR)/libc/%.o: ../libc/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(LIBC_CFLAGS) -c $< -o $@

# Link all tests together
$(UNIT_BIN): $(ALL_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR):
	@mkdir -p $@

clean:
	@rm -rf $(BUILD_DIR)

list:
	@echo "Discovered unit tests:"
	@for test in $(UNIT_TESTS); do echo "  $$test"; done
	@echo ""
	@echo "libc files linked:"
	@for src in $(LIBC_SRCS); do echo "  $$src"; done

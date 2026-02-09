include ../config.mk

CFLAGS  := --sysroot=$(SYSROOT) -isystem $(SYSROOT)/usr/include \
           -ffreestanding -O2 -g -Wall -Wextra -nostdlib -mgeneral-regs-only
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
LDFLAGS := -T link.ld -nostdlib --sysroot=$(SYSROOT)
LIBS    := -L$(SYSROOT)/usr/lib -lc -lgcc

SRCS  := $(wildcard *.c *.cpp)
PROGS := $(addprefix $(BUILDDIR)/user/, $(addsuffix .elf, $(basename $(SRCS))))

.PHONY: all clean

all: $(PROGS)

$(BUILDDIR)/user/%.elf: $(BUILDDIR)/user/crt0.o $(BUILDDIR)/user/%.o link.ld
	@mkdir -p $(dir $@)
	@$(CC) $(LDFLAGS) -o $@ $(BUILDDIR)/user/crt0.o $(BUILDDIR)/user/$*.o $(LIBS)

$(BUILDDIR)/user/crt0.o: crt0.S
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/user/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/user/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -f $(BUILDDIR)/user/*.o $(BUILDDIR)/user/*.elf

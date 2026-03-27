include ../config.mk

CFLAGS  := $(USER_CFLAGS) -mgeneral-regs-only
CXXFLAGS := $(USER_CXXFLAGS) -fno-rtti -fno-exceptions
LDFLAGS := -T link.ld -nostdlib --sysroot=$(SYSROOT)
LIBS    := -L$(SYSROOT)/usr/lib -lc -lgcc

SRCS  := $(wildcard *.c *.cpp)
PROGS := $(addprefix $(BUILDDIR)/user/, $(addsuffix .elf, $(basename $(SRCS))))

OBJS := $(addprefix $(BUILDDIR)/user/, crt0.o $(addsuffix .o, $(basename $(SRCS))))

.PHONY: all clean
.SECONDARY: $(OBJS)

all: $(PROGS)
	@if [ -d doom/doomgeneric ]; then \
	    $(MAKE) -C doom BUILDDIR=$(BUILDDIR) SYSROOT=$(SYSROOT); \
	fi

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
	@$(CPP) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -f $(BUILDDIR)/user/*.o $(BUILDDIR)/user/*.elf

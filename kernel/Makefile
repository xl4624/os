include ../config.mk

# ==== Build Settings ====
CFLAGS := $(BASE_CFLAGS) -mgeneral-regs-only
CPPFLAGS := -fno-exceptions -fno-rtti -fno-threadsafe-statics -D__is_kernel -I$(CURDIR)/include

# ==== Source and Object Files ====
SRCS_CPP := $(shell find . -type f -name "*.cpp" ! -path "./test/*")
ifdef KERNEL_TESTS
CPPFLAGS += -DKERNEL_TESTS
SRCS_CPP += $(shell find ./test -type f -name "*.cpp")
ifdef KTEST_RUN
CPPFLAGS += -DKTEST_RUN
endif
endif
SRCS_ASM := $(shell find . -type f -name "*.S")
OBJS_CPP := $(patsubst ./%.cpp,$(BUILDDIR)/kernel/%.o,$(SRCS_CPP))
OBJS_ASM := $(patsubst ./%.S,$(BUILDDIR)/kernel/%.o,$(SRCS_ASM))
OBJS     := $(OBJS_CPP) $(OBJS_ASM)

.PHONY: all clean install

all: $(OBJS)

$(BUILDDIR)/kernel/%.o: ./%.cpp
	@mkdir -p $(dir $@)
	$(CPP) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILDDIR)/kernel/%.o: ./%.S
	@mkdir -p $(dir $@)
	$(AS) $< -o $@

clean:
	rm -f $(OBJS)

install:
	mkdir -p $(SYSROOT)$(INCLUDEDIR)
	cp -R -p include/. $(SYSROOT)$(INCLUDEDIR)/.

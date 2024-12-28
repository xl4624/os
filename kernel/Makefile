include ../config.mk

# ==== Build Settings ====
CFLAGS := $(BASE_CFLAGS) -mgeneral-regs-only
CPPFLAGS := -fno-exceptions -fno-rtti -fno-threadsafe-statics -D__is_kernel -Iinclude

# ==== Source and Object Files ====
SRCS_CPP := $(shell find . -type f -name "*.cpp")
SRCS_ASM := $(shell find . -type f -name "*.S")
OBJS_CPP := $(SRCS_CPP:%.cpp=%.o)
OBJS_ASM := $(SRCS_ASM:%.S=%.o)
OBJS     := $(OBJS_CPP) $(OBJS_ASM)

.PHONY: all clean install

all: $(OBJS)

%.o: %.cpp
	$(CPP) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

%.o: %.S
	$(AS) $< -o $@

clean:
	rm -f $(OBJS)

install:
	mkdir -p $(SYSROOT)$(INCLUDEDIR)
	cp -R -p include/. $(SYSROOT)$(INCLUDEDIR)/.

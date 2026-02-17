include ../config.mk

CFLAGS := ${BASE_CFLAGS} -Iinclude
CPPFLAGS := -fno-exceptions -fno-rtti -fno-threadsafe-statics

SRCS := $(shell find . -type f -name "*.cpp")
OBJS := $(patsubst ./%.cpp,$(BUILDDIR)/libcpp/%.o,$(SRCS))
TARGET := $(BUILDDIR)/libcpp/libcpp.a

.PHONY: all clean install

all: $(TARGET)

$(BUILDDIR)/libcpp/%.o: ./%.cpp
	@mkdir -p $(dir $@)
	$(CPP) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

clean:
	rm -f $(OBJS) $(TARGET)

install: install-headers install-libs

install-headers:
	mkdir -p $(SYSROOT)$(INCLUDEDIR)
	cp -R -p include/. $(SYSROOT)$(INCLUDEDIR)/.

install-libs: $(TARGET)
	mkdir -p $(SYSROOT)$(LIBDIR)
	cp $(TARGET) $(SYSROOT)$(LIBDIR)

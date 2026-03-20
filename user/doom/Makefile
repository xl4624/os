include ../../config.mk

CFLAGS_COMMON := --sysroot=$(SYSROOT) -isystem $(SYSROOT)/usr/include/c \
    -ffreestanding -O2 -g -nostdlib -D__is_libc
LDFLAGS := -T ../link.ld -nostdlib --sysroot=$(SYSROOT)
LIBS    := -L$(SYSROOT)/usr/lib -lc -lgcc

DGDIR   := doomgeneric/doomgeneric
EXCLUDE_DG := doomgeneric_allegro.c doomgeneric_emscripten.c doomgeneric_linuxvt.c \
              doomgeneric_sdl.c doomgeneric_win.c doomgeneric_xlib.c \
              doomgeneric_soso.c doomgeneric_sosox.c \
              i_allegromusic.c i_allegrosound.c i_cdmus.c \
              i_sdlmusic.c i_sdlsound.c
DG_SRCS := $(filter-out $(addprefix $(DGDIR)/,$(EXCLUDE_DG)), $(wildcard $(DGDIR)/*.c))
DG_OBJS := $(patsubst $(DGDIR)/%.c, $(BUILDDIR)/user/doom/dg/%.o, $(DG_SRCS))
MY_OBJ  := $(BUILDDIR)/user/doom/dg_myos.o
CRT0    := $(BUILDDIR)/user/crt0.o
DOOM_ELF := $(BUILDDIR)/user/doom.elf

.PHONY: all clean
all: $(DOOM_ELF)

$(DOOM_ELF): $(CRT0) $(DG_OBJS) $(MY_OBJ) ../link.ld
	@mkdir -p $(dir $@)
	@$(CC) $(LDFLAGS) -o $@ $(CRT0) $(DG_OBJS) $(MY_OBJ) $(LIBS)

$(BUILDDIR)/user/doom/dg/%.o: $(DGDIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS_COMMON) -std=gnu11 -w -include stdio.h -I$(DGDIR) -c $< -o $@

$(BUILDDIR)/user/doom/dg_myos.o: dg_myos.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS_COMMON) -I$(DGDIR) -c $< -o $@

clean:
	@rm -rf $(BUILDDIR)/user/doom/ $(DOOM_ELF)

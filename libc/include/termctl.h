#ifndef _TERMCTL_H
#define _TERMCTL_H

#include <sys/cdefs.h>

/* VGA text-mode dimensions */
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

/* VGA text-mode color constants */
#define VGA_BLACK 0
#define VGA_BLUE 1
#define VGA_GREEN 2
#define VGA_CYAN 3
#define VGA_RED 4
#define VGA_MAGENTA 5
#define VGA_BROWN 6
#define VGA_LIGHT_GREY 7
#define VGA_DARK_GREY 8
#define VGA_LIGHT_BLUE 9
#define VGA_LIGHT_GREEN 10
#define VGA_LIGHT_CYAN 11
#define VGA_LIGHT_RED 12
#define VGA_LIGHT_MAGENTA 13
#define VGA_LIGHT_BROWN 14
#define VGA_WHITE 15

/* Combine foreground and background into a single color byte */
#define VGA_COLOR(fg, bg) ((fg) | ((bg) << 4))

__BEGIN_DECLS

void set_cursor(unsigned int row, unsigned int col);
void set_color(unsigned char color);
void clear_screen(void);

__END_DECLS

#endif /* _TERMCTL_H */

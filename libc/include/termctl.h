#ifndef _TERMCTL_H
#define _TERMCTL_H

#include <sys/cdefs.h>

/* Terminal dimensions */
#define TERM_WIDTH 80
#define TERM_HEIGHT 25

/* Color constants */
#define TERM_BLACK 0
#define TERM_RED 1
#define TERM_GREEN 2
#define TERM_YELLOW 3
#define TERM_BLUE 4
#define TERM_MAGENTA 5
#define TERM_CYAN 6
#define TERM_WHITE 7
#define TERM_BRIGHT_BLACK 8
#define TERM_BRIGHT_RED 9
#define TERM_BRIGHT_GREEN 10
#define TERM_BRIGHT_YELLOW 11
#define TERM_BRIGHT_BLUE 12
#define TERM_BRIGHT_MAGENTA 13
#define TERM_BRIGHT_CYAN 14
#define TERM_BRIGHT_WHITE 15

/* Combine foreground and background into a single color byte */
#define TERM_COLOR(fg, bg) ((fg) | ((bg) << 4))

__BEGIN_DECLS

void set_cursor(unsigned int row, unsigned int col);
void set_color(unsigned char color);
void show_cursor(void);
void hide_cursor(void);
void clear_screen(void);

__END_DECLS

#endif /* _TERMCTL_H */

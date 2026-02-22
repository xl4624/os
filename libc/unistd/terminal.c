#include <termctl.h>

#if defined(__is_libk)

    #include "tty.h"

void set_cursor(unsigned int row, unsigned int col) {
    terminal_set_position(row, col);
}

void set_color(unsigned char color) {
    terminal_set_color(color);
}

void clear_screen(void) {
    terminal_clear();
}

#elif defined(__is_libc)

    #include <unistd.h>

// VGA index to ANSI color code tables.
static const unsigned char fg_table[16] = {30, 34, 32, 36, 31, 35, 33, 37,
                                           90, 94, 92, 96, 91, 95, 93, 97};
static const unsigned char bg_table[16] = {
    40, 44, 42, 46, 41, 45, 43, 47, 100, 104, 102, 106, 101, 105, 103, 107};

// Write an unsigned integer as decimal digits into buf, returning the new
// end pointer. Does not null-terminate; always writes at least one digit.
static char *write_uint(char *p, unsigned int n) {
    if (n >= 100u)
        *p++ = (char)('0' + (int)(n / 100u));
    if (n >= 10u)
        *p++ = (char)('0' + (int)((n / 10u) % 10u));
    *p++ = (char)('0' + (int)(n % 10u));
    return p;
}

void set_cursor(unsigned int row, unsigned int col) {
    char buf[16];
    char *p = buf;
    *p++ = '\033';
    *p++ = '[';
    p = write_uint(p, row + 1u);
    *p++ = ';';
    p = write_uint(p, col + 1u);
    *p++ = 'H';
    (void)write(1, buf, (size_t)(p - buf));
}

void set_color(unsigned char color) {
    char buf[32];
    char *p = buf;
    unsigned int fg = (unsigned int)(color & 0x0Fu);
    unsigned int bg = (unsigned int)((unsigned int)(color >> 4) & 0x0Fu);
    *p++ = '\033';
    *p++ = '[';
    p = write_uint(p, (unsigned int)bg_table[bg]);
    *p++ = ';';
    p = write_uint(p, (unsigned int)fg_table[fg]);
    *p++ = 'm';
    (void)write(1, buf, (size_t)(p - buf));
}

void clear_screen(void) {
    (void)write(1, "\033[2J\033[H", 7);
}

#endif

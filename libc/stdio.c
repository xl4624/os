#include <stdio.h>

#include "tty.h"

// TODO: Implement
int printf(const char *__restrict format, ...) {
    return -1;
}

int putchar(int c) {
#if defined(__is_libk)
    terminal_putchar((char)c);
#elif defined(__is_libc)
    // TODO: Implement stdio and the write system call.
#endif

    return c;
}

// TODO: Implement
int puts(const char *s) {
    return -1;
}

#include <stdio.h>

#include "tty.h"

int putchar(int c) {
#if defined(__is_libk)
    terminal_putchar((char)c);
#elif defined(__is_libc)
    // TODO: Implement stdio and the write system call.
#endif

    return c;
}

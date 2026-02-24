#include <stdio.h>

#if defined(__is_libk)
    #include "tty.h"

int putchar(int c) {
    terminal_putchar((char)c);
    return c;
}
#elif defined(__is_libc)
    #include <unistd.h>

int putchar(int c) {
    char ch = (char)c;
    write(1, &ch, 1);
    return c;
}
#endif

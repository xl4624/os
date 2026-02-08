#include <stdio.h>

#if defined(__is_libk)
    #include "tty.h"
#elif defined(__is_libc)
    #include <unistd.h>
#endif

int putchar(int c) {
#if defined(__is_libk)
    terminal_putchar((char)c);
#elif defined(__is_libc)
    char ch = (char)c;
    write(1, &ch, 1);
#endif

    return c;
}

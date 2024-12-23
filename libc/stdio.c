#include <stdarg.h>
#include <stdio.h>

#include "tty.h"

int printf(const char *__restrict format, ...) {
    va_list args;
    va_start(args, format);
    char *s;

    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 's':
                    s = va_arg(args, char *);
                    puts(s);
                    break;
                // TODO: support other formats
                default:
                    putchar('%');
                    putchar(*format);
            }
        } else {
            putchar(*format);
        }
        format++;
    }

    va_end(args);
    return 1;  // TODO: should return number of bytes printed
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
    while (*s) {
        if (putchar(*s++) == EOF) {
            return EOF;
        }
    }
    // Append trailing newline and return nonnegative number on success,
    // see puts(3)
    if (putchar('\n') == EOF) {
        return EOF;
    }
    return 1;
}

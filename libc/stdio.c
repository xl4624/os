#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "tty.h"

// NOTE: Helper function for printf, does not append newline
int print(const char *s, size_t n) {
    const unsigned char *bytes = (const unsigned char *)s;
    for (size_t i = 0; i < n; i++)
        if (putchar(bytes[i]) == EOF)
            return EOF;
    return 1;
}

int printf(const char *__restrict format, ...) {
    va_list args;
    va_start(args, format);

    int written = 0;
    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 's':
                    char *s = va_arg(args, char *);
                    size_t len = strlen(s);
                    if (print(s, len) == EOF)
                        goto failure;

                    written += len;
                    break;
                case 'c':
                    // cast here since va_arg only takes fully promoted types
                    char c = (char)va_arg(args, int);
                    putchar(c);

                    written++;
                    break;
                case '%':
                    putchar('%');

                    written++;
                    break;
                // TODO: support other formats
                default:
                    putchar('%');
                    putchar(*format);

                    written += 2;
                    break;
            }
        } else {
            putchar(*format);
            written++;
        }
        format++;
    }

    va_end(args);
    return written;

failure:
    va_end(args);
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

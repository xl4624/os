#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tty.h"

int print(const char *s, size_t n) {
    const unsigned char *bytes = (const unsigned char *)s;
    for (size_t i = 0; i < n; i++) {
        if (putchar(bytes[i]) == EOF)
            return EOF;
    }
    return 1;
}

int printf(const char *__restrict format, ...) {
    va_list args;
    va_start(args, format);

    int written = 0;  // number of bytes written
    size_t len;       // temp length storage

    char ch;
    while ((ch = *format++)) {
        if (ch == '%') {
            switch (*format++) {
                case 's':
                    char *s = va_arg(args, char *);
                    if (s == NULL)
                        s = "(null)";
                    len = strlen(s);
                    if (print(s, len) == EOF)
                        goto failure;

                    written += len;
                    break;
                case 'c':
                    // cast here since va_arg only takes fully promoted types
                    char c = (char)va_arg(args, int);
                    if (putchar(c) == EOF)
                        goto failure;

                    written++;
                    break;
                case 'd':
                    int n = va_arg(args, int);
                    char buffer[12];  // enough for 32-bit integer
                    itoa(n, buffer);
                    len = strlen(buffer);
                    if (print(buffer, len) == EOF)
                        goto failure;

                    written += len;
                    break;
                // TODO: support other formats
                case '%':
                    if (putchar('%'))
                        goto failure;

                    written++;
                    break;
                default:
                    if (putchar('%') == EOF || putchar(ch) == EOF)
                        goto failure;

                    written += 2;
                    break;
            }
        } else {
            putchar(ch);
            written++;
        }
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
    return printf("%s\n", s);
}

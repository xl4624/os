#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>
#include <sys/cdefs.h>

#define EOF (-1)

__BEGIN_DECLS

int vprintf(const char* __restrict__ format, va_list ap);
int printf(const char* __restrict__ format, ...);
int getchar(void);
int putchar(int c);
int puts(const char* s);

__END_DECLS

#endif /* _STDIO_H */

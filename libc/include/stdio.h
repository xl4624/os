#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>
#include <stddef.h>
#include <sys/cdefs.h>

#define EOF (-1)

__BEGIN_DECLS

int vprintf(const char* __restrict__ format,
            va_list ap);  // NOLINT(misc-const-correctness)
int printf(const char* __restrict__ format, ...);
int vsnprintf(char* buf, size_t size, const char* __restrict__ format, va_list ap);
int snprintf(char* buf, size_t size, const char* __restrict__ format, ...);
int vsprintf(char* buf, const char* __restrict__ format, va_list ap);
int sprintf(char* buf, const char* __restrict__ format, ...);
int getchar(void);
int putchar(int c);
int puts(const char* s);

int sscanf(const char* str, const char* format, ...);

__END_DECLS

#endif /* _STDIO_H */

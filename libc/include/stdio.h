#ifndef _STDIO_H
#define _STDIO_H

#include <sys/cdefs.h>

#define EOF (-1)

__BEGIN_DECLS

int vprint(const char *__restrict__ format, ...);
int printf(const char *__restrict__ format, ...);
int putchar(int c);
int puts(const char *s);

__END_DECLS

#endif

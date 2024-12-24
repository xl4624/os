#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char *__restrict format, ...);
int putchar(int c);
int puts(const char *s);

#ifdef __cplusplus
}
#endif

#endif

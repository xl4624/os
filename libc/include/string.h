#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

// void *memccpy(void *__restrict__, const void *__restrict__, int, size_t);
// void *memchr(const void *, int, size_t);
int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *__restrict__ dest, const void *__restrict__ src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
// char *stpcpy(char *__restrict__, const char *__restrict__);
// char *stpncpy(char *__restrict__, const char *__restrict__, size_t);
// char *strcat(char *__restrict__, const char *__restrict__);
char *strchr(const char *s, int c);
// int strcmp(const char *, const char *);
// int strcoll(const char *, const char *);
// int strcoll_l(const char *, const char *, locale_t);
char *strcpy(char *__restrict__ dst, const char *__restrict__ src);
// size_t strcspn(const char *, const char *);
// char *strdup(const char *);
// char *strerror(int);
// char *strerror_l(int, locale_t);
// int strerror_r(int, char *, size_t);
size_t strlen(const char *s);
// char *strncat(char *__restrict__, const char *__restrict__, size_t);
// int strncmp(const char *, const char *, size_t);
// char *strncpy(char *__restrict__, const char *__restrict__, size_t);
// char *strndup(const char *, size_t);
size_t strnlen(const char *, size_t);
// char *strpbrk(const char *, const char *);
char *strrchr(const char *s, int c);
// char *strsignal(int);
// size_t strspn(const char *, const char *);
// char *strstr(const char *, const char *);
// char *strtok(char *__restrict__, const char *__restrict__);
// char *strtok_r(char *__restrict__, const char *__restrict__, char
// **__restrict__); size_t strxfrm(char *__restrict__, const char *__restrict__,
// size_t); size_t strxfrm_l(char *__restrict__, const char *__restrict__,
// size_t, locale_t);

__END_DECLS

#endif /* _STRING_H */

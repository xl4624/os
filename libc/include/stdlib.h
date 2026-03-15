#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

__attribute__((noreturn)) void abort(void);
__attribute__((noreturn)) void exit(int status);
void itoa(int value, char* str, int base);

void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);

#define RAND_MAX 0x7FFF

int abs(int x);
int rand(void);
void srand(unsigned int seed);

long __libc_strtol(const char* nptr, char** endptr, int base);
unsigned long __libc_strtoul(const char* nptr, char** endptr, int base);
int __libc_atoi(const char* nptr);

#define strtol __libc_strtol
#define strtoul __libc_strtoul
#define atoi __libc_atoi

char* getenv(const char* name);
int system(const char* command);

__END_DECLS

#endif

#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

__attribute__((noreturn)) void abort(void);
void itoa(int value, char* str, int base);

void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);

#define RAND_MAX 0x7FFF

int abs(int x);
int rand(void);
void srand(unsigned int seed);

__END_DECLS

#endif

#ifndef _STDLIB_H
#define _STDLIB_H

#include <sys/cdefs.h>

__BEGIN_DECLS

__attribute__((noreturn)) void abort(void);
void itoa(int value, char *str, int base);

__END_DECLS

#endif

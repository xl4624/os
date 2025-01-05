#ifndef _STDLIB_H
#define _STDLIB_H

#include <sys/cdefs.h>

__BEGIN_DECLS

__attribute__((__noreturn__)) void abort(void);
void itoa(int n, char *s);

__END_DECLS

#endif

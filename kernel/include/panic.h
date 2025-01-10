#ifndef PANIC_H
#define PANIC_H

#include <sys/cdefs.h>

__BEGIN_DECLS

__attribute__((noreturn)) void panic(const char *format, ...);

__END_DECLS

#endif /* PANIC_H */

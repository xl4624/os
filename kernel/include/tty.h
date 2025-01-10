#ifndef TTY_H
#define TTY_H

#include <stddef.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

void terminal_write(const char *data);
void terminal_putchar(char c);

__END_DECLS

#endif /* TTY_H */

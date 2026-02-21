#ifndef _UNISTD_H
#define _UNISTD_H

#include <stddef.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

__attribute__((noreturn)) void _exit(int status);
int write(int fd, const void *buf, size_t count);
int read(int fd, void *buf, size_t count);
unsigned int sleep(unsigned int seconds);
void msleep(unsigned int ms);
void *sbrk(int increment);

__END_DECLS

#endif

#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H

/*
 * System call numbers shared between kernel and userspace.
 *
 * These values are indices into the kernel's syscall dispatch table.
 * Keep in sync with the handlers in kernel/cpu/syscall.cpp.
 */

#define SYS_EXIT 0
#define SYS_READ 1
#define SYS_WRITE 2
#define SYS_SLEEP 3
#define SYS_SBRK 4
#define SYS_MAX 5

#endif

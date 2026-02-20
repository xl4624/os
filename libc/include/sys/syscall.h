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
#define SYS_SET_CURSOR 5
#define SYS_SET_COLOR 6
#define SYS_CLEAR 7
#define SYS_GETPID 8
#define SYS_EXEC 9
#define SYS_MAX 10

#endif

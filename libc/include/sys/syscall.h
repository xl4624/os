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
#define SYS_GETPID 5
#define SYS_EXEC 6
#define SYS_FORK 7
#define SYS_WAITPID 8
#define SYS_PIPE 9
#define SYS_CLOSE 10
#define SYS_DUP2 11
#define SYS_SHMGET 12
#define SYS_SHMAT 13
#define SYS_SHMDT 14
#define SYS_MAX 15

#endif

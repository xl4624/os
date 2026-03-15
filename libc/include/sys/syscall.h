#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H

/*
 * System call numbers shared between kernel and userspace.
 *
 * These values are indices into the kernel's syscall dispatch table.
 * Keep in sync with the handlers in kernel/cpu/syscall.cpp.
 */

#define SYS_EXIT 0      /* Linux:  1 */
#define SYS_FORK 1      /* Linux:  2 */
#define SYS_READ 2      /* Linux:  3 */
#define SYS_WRITE 3     /* Linux:  4 */
#define SYS_OPEN 4      /* Linux:  5 */
#define SYS_CLOSE 5     /* Linux:  6 */
#define SYS_WAITPID 6   /* Linux:  7 */
#define SYS_EXEC 7      /* Linux: 11 (execve) */
#define SYS_LSEEK 8     /* Linux: 19 */
#define SYS_GETPID 9    /* Linux: 20 */
#define SYS_PIPE 10     /* Linux: 42 */
#define SYS_SBRK 11     /* Linux: 45 (brk) */
#define SYS_DUP2 12     /* Linux: 63 */
#define SYS_SLEEP 13    /* custom */
#define SYS_SHMGET 14   /* custom */
#define SYS_SHMAT 15    /* custom */
#define SYS_SHMDT 16    /* custom */
#define SYS_GETTICKS 17 /* custom */
#define SYS_FB_FLIP 18  /* custom */
#define SYS_MAX 19

#endif

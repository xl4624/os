#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H

/*
 * System call numbers shared between kernel and userspace.
 *
 * These values are indices into the kernel's syscall dispatch table.
 * Keep in sync with the handlers in kernel/cpu/syscall.cpp.
 */

#define SYS_EXIT 0       /* Linux:  1 */
#define SYS_FORK 1       /* Linux:  2 */
#define SYS_READ 2       /* Linux:  3 */
#define SYS_WRITE 3      /* Linux:  4 */
#define SYS_OPEN 4       /* Linux:  5 */
#define SYS_CLOSE 5      /* Linux:  6 */
#define SYS_WAITPID 6    /* Linux:  7 */
#define SYS_EXEC 7       /* Linux: 11 (execve) */
#define SYS_LSEEK 8      /* Linux: 19 */
#define SYS_GETPID 9     /* Linux: 20 */
#define SYS_PIPE 10      /* Linux: 42 */
#define SYS_SBRK 11      /* Linux: 45 (brk) */
#define SYS_DUP2 12      /* Linux: 63 */
#define SYS_SLEEP 13     /* custom */
#define SYS_SHMGET 14    /* custom */
#define SYS_SHMAT 15     /* custom */
#define SYS_SHMDT 16     /* custom */
#define SYS_GETTICKS 17  /* custom */
#define SYS_FB_FLIP 18   /* custom */
#define SYS_KILL 19      /* custom */
#define SYS_SIGACTION 20 /* custom */
#define SYS_SIGRETURN 21 /* custom */
#define SYS_CHDIR 22     /* custom */
#define SYS_GETCWD 23    /* custom */
#define SYS_GETDENTS 24  /* custom */
#define SYS_IOCTL 25     /* custom */
#define SYS_STAT 26      /* custom */
#define SYS_FSTAT 27     /* custom */
#define SYS_MKDIR 28     /* custom */
#define SYS_UNLINK 29    /* custom */
#define SYS_RENAME 30    /* custom */
#define SYS_FCNTL 31     /* custom */
#define SYS_MOUNT 32     /* custom */
#define SYS_UMOUNT 33    /* custom */
#define SYS_MAX 34

#include <stdint.h>

// Translates a raw kernel return value to the POSIX convention:
// returns r on success (r >= 0), sets errno and returns -1 on failure (r < 0).
// In libk (kernel) context no errno is available, so the raw value is returned.
#ifdef __is_libc
#include <errno.h>
static inline int32_t __syscall_ret(int32_t r) {
  if (r < 0) {
    errno = -r;
    return -1;
  }
  return r;
}
#else
// NOLINTNEXTLINE(bugprone-reserved-identifier)
static inline int32_t __syscall_ret(int32_t r) { return r; }
#endif

#endif

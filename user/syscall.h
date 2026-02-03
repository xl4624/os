#pragma once

/*
 * Minimal syscall wrappers for userspace programs.
 *
 * Syscall ABI (int 0x80):
 *   EAX = syscall number
 *   EBX = arg1,  ECX = arg2,  EDX = arg3
 *   Return value in EAX.
 */

#define SYS_EXIT  0
#define SYS_SLEEP 1
#define SYS_WRITE 2
#define SYS_READ  3

static inline int syscall1(int num, int arg1) {
    int ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(num), "b"(arg1));
    return ret;
}

static inline int syscall3(int num, int arg1, int arg2, int arg3) {
    int ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

static inline void exit(int code) {
    syscall1(SYS_EXIT, code);
    __builtin_unreachable();
}

static inline int write(int fd, const void *buf, int count) {
    return syscall3(SYS_WRITE, fd, (int)buf, count);
}

static inline int read(int fd, void *buf, int count) {
    return syscall3(SYS_READ, fd, (int)buf, count);
}

static inline void sleep(int ms) {
    syscall1(SYS_SLEEP, ms);
}

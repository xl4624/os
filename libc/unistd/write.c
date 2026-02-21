#include <unistd.h>

#if defined(__is_libk)

int write(int fd, const void *buf, size_t count) {
    (void)fd;
    (void)buf;
    (void)count;
    return -1;
}

#elif defined(__is_libc)

    #include <sys/syscall.h>

int write(int fd, const void *buf, size_t count) {
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_WRITE), "b"(fd), "c"(buf), "d"(count));
    return ret;
}

#endif

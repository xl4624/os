#include <unistd.h>

#if defined(__is_libk)

void msleep(unsigned int ms) {
    (void)ms;
}

#elif defined(__is_libc)

    #include <sys/syscall.h>

void msleep(unsigned int ms) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_SLEEP), "b"(ms));
    (void)ret;
}

#endif

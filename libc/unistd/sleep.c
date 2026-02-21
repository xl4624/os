#include <unistd.h>

#if defined(__is_libk)

unsigned int sleep(unsigned int seconds) {
    (void)seconds;
    return 0;
}

#elif defined(__is_libc)

    #include <sys/syscall.h>

unsigned int sleep(unsigned int seconds) {
    unsigned int ms = seconds * 1000;
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_SLEEP), "b"(ms));
    (void)ret;
    return 0;
}

#endif

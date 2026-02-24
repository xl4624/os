#include <unistd.h>

#if defined(__is_libk)

void _exit(int status) {
    (void)status;
    __builtin_unreachable();
}

#elif defined(__is_libc)

    #include <sys/syscall.h>

void _exit(int status) {
    asm volatile("int $0x80" ::"a"(SYS_EXIT), "b"(status));
    __builtin_unreachable();
}

#endif

#include <stdio.h>
#include <stdlib.h>

__attribute__((__noreturn__)) void abort(void) {
#if defined(__is_libk)
    // TODO: Add proper kernel panic
    printf("kernel: panic: abort()\n");
    asm volatile("hlt");
#else
    // TODO: Terminate the process using SIGABRT
    printf("abort()\n");
#endif
    while (1) {}
    __builtin_unreachable();
}

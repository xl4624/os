#include <stdio.h>
#include <stdlib.h>

__attribute__((noreturn)) void abort(void) {
#if defined(__is_libk)
    // TODO: Add proper kernel panic
    printf("kernel: panic: abort()\n");
    asm("cli; hlt");
#else
    // TODO: Terminate the process using SIGABRT
    printf("abort()\n");
    while (1) {}
    __builtin_unreachable();
#endif
}

#include <stdio.h>
#include <stdlib.h>

#include "panic.h"

__attribute__((noreturn)) void abort(void) {
#if defined(__is_libk)
    panic("kernel: abort()\n");
#else
    // TODO: Terminate the process using SIGABRT
    printf("abort()\n");
    while (1) {
        asm volatile("hlt" : : : "memory");
    }
#endif
    __builtin_unreachable();
}

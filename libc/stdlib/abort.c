#include <stdio.h>
#include <stdlib.h>

#include "interrupt.h"
#include "panic.h"

__attribute__((noreturn)) void abort(void) {
#if defined(__is_libk)
    panic("kernel: abort()\n");
#else
    // TODO: Terminate the process using SIGABRT
    printf("abort()\n");
    halt_and_catch_fire();
#endif
    __builtin_unreachable();
}

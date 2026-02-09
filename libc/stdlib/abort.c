#include <stdlib.h>

#if defined(__is_libk)
    #include "interrupt.h"
    #include "panic.h"
#elif defined(__is_libc)
    #include <unistd.h>
#endif

__attribute__((noreturn)) void abort(void) {
#if defined(__is_libk)
    panic("kernel: abort()\n");
#elif defined(__is_libc)
    _exit(134);
#endif
    __builtin_unreachable();
}

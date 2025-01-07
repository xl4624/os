#include "panic.h"

#include <stdarg.h>
#include <stdio.h>

[[noreturn]] void panic(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("PANIC: ");
    vprintf(format, args);
    va_end(args);

    asm("cli; hlt");
    __builtin_unreachable();
}

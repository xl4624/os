#include "panic.h"

#include <stdarg.h>
#include <stdio.h>

#include "interrupt.hpp"

__attribute__((noreturn)) void panic(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("panic: ");
    vprintf(format, args);
    va_end(args);

    halt_and_catch_fire();
}

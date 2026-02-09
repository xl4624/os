#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "panic.h"

// clang-format off
__attribute__((noreturn))
void __assert_fail(const char *__assertion, const char *__file,
                   unsigned int __line, const char *__function) {
// clang-format on
#ifdef __is_libk
    panic("%s:%d: %s: Assertion `%s' failed.\n", __file, __line, __function,
          __assertion);
#else
    printf("Assertion failed: %s, file %s, line %u, function %s\n", __assertion,
           __file, __line, __function);
    abort();
#endif
    __builtin_unreachable();
}

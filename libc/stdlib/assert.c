#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// clang-format off
__attribute__((noreturn))
void __assert_fail(const char *_assertion, const char *_file,
                   unsigned int _line, const char *_function) {
// clang-format on
#ifdef __is_libk
#include "panic.h"

  panic("%s:%d: %s: Assertion `%s' failed.\n", _file, _line, _function, _assertion);
#else
  printf("Assertion failed: %s, file %s, line %u, function %s\n", _assertion, _file, _line,
         _function);
  abort();
#endif
  __builtin_unreachable();
}

#include <assert.h>
#include <panic.h>

// clang-format off
__attribute__((noreturn))
void __assert_fail(const char *__assertion, const char *__file,
                   unsigned int __line, const char *__function) {
// clang-format on
#ifdef __is_libk
    panic("%s:%d: %s: Assertion `%s' failed.\n", __file, __line, __function,
          __assertion);
#else
    (void)__assertion, (void)__file, (void)__line, (void)__function;
    // TODO: Terminate the process using SIGABRT
    while (1) {}
#endif
    __builtin_unreachable();
}

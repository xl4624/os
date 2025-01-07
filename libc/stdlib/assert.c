#include <assert.h>
#include <panic.h>

__attribute__((noreturn))
void __assert_fail(const char *__assertion, const char *__file,
                   unsigned int __line, const char *__function) {
#ifdef __is_libk
    panic("%s:%d: %s: Assertion `%s' failed.\n", __file, __line, __function,
          __assertion);
    asm volatile("cli; hlt");
#else
    // TODO: Terminate the process using SIGABRT
    while (1) {}
    __builtin_unreachable();
#endif
}

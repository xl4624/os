#pragma once

#include "../../tests/framework/test.h"

#ifdef KERNEL_TESTS
// Write assertion failure details to QEMU debugcon (port 0xE9) so that
// test output can be captured to a file on the host.
void ktest_dbg_assert_fail(const char *expr, const char *file, int line);

    #undef ASSERT
    #define ASSERT(expr)                                                     \
        do {                                                                 \
            if (!(expr)) {                                                   \
                printf("FAILED\n    Assertion failed: %s at %s:%d\n", #expr, \
                       __FILE__, __LINE__);                                  \
                ktest_dbg_assert_fail(#expr, __FILE__, __LINE__);            \
                kTestState.current_failed = true;                            \
                return;                                                      \
            }                                                                \
        } while (false)
#endif

namespace KTest {

    void run_all();

}

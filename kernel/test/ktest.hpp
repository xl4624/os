#pragma once

#include <stddef.h>
#include <stdio.h>

namespace KTest {
    void run_all();
}

struct KTestCase {
    const char *name;
    void (*func)();
};

struct KTestState {
    int passed = 0;
    int failed = 0;
    bool current_failed = false;
};

constexpr int KTEST_MAX = 128;

extern KTestCase g_ktests[KTEST_MAX];
extern int g_ktest_count;
extern KTestState g_ktest_state;

struct KTestRegistrar {
    KTestRegistrar(const char *name, void (*func)()) {
        if (g_ktest_count < KTEST_MAX)
            g_ktests[g_ktest_count++] = {name, func};
    }
};

// ── Assertion macros
// ──────────────────────────────────────────────────────────

#define KASSERT(expr)                                                     \
    do {                                                                  \
        if (!(expr)) {                                                    \
            printf("    FAIL: %s  (%s:%d)\n", #expr, __FILE__, __LINE__); \
            g_ktest_state.current_failed = true;                          \
            return;                                                       \
        }                                                                 \
    } while (0)

#define KASSERT_EQ(a, b) KASSERT((a) == (b))
#define KASSERT_NE(a, b) KASSERT((a) != (b))
#define KASSERT_NULL(p) KASSERT((p) == nullptr)
#define KASSERT_NOT_NULL(p) KASSERT((p) != nullptr)

// ── Test registration macro
// ───────────────────────────────────────────────────
//
// Usage:  KTEST(category, test_name) { /* body */ }
//
// The static KTestRegistrar runs before kernel_main() (via _init) and
// records the test in g_ktests[].

#define KTEST(category, name)                                                  \
    static void ktest_##category##_##name();                                   \
    static KTestRegistrar kreg_##category##_##name(#category "/" #name,        \
                                                   ktest_##category##_##name); \
    static void ktest_##category##_##name()

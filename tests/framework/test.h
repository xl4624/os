#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct TestCase {
    const char *name;
    void (*func)();
    const char *file;
    int line;
};

static constexpr int kMaxTests = 256;

// The list of registered test cases.
extern TestCase kTests[kMaxTests];
// The number of registered test cases.
extern int kTestCount;

struct TestRegistrar {
    TestRegistrar(const char *name, void (*func)(), const char *file,
                  int line) {
        if (kTestCount >= kMaxTests) {
            printf("ERROR: Too many tests! Increase kMaxTests (currently %d)\n",
                   kMaxTests);
            printf("Failed to register: %s\n", name);
            return;
        }
        kTests[kTestCount++] = {name, func, file, line};
    }
};

struct TestState {
    int passed = 0;
    int failed = 0;
    const char *current_test = nullptr;
    bool current_failed = false;
};

// The current test execution state.
extern TestState kTestState;

#define ASSERT(expr)                                                     \
    do {                                                                 \
        if (!(expr)) {                                                   \
            printf("FAILED\n    Assertion failed: %s at %s:%d\n", #expr, \
                   __FILE__, __LINE__);                                  \
            kTestState.current_failed = true;                            \
            return;                                                      \
        }                                                                \
    } while (false)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_TRUE(expr) ASSERT(expr)
#define ASSERT_FALSE(expr) ASSERT(!(expr))
#define ASSERT_NULL(p) ASSERT((p) == nullptr)
#define ASSERT_NOT_NULL(p) ASSERT((p) != nullptr)
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0)

#define TEST(category, name)                                                \
    static void test_##category##_##name();                                 \
    static TestRegistrar registrar_##category##_##name(                     \
        #category "/" #name, test_##category##_##name, __FILE__, __LINE__); \
    static void test_##category##_##name()

int run_all_tests();

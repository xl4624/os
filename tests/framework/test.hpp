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

// TODO: consider using a dynamic array or linked list
#define MAX_TESTS 128
extern TestCase g_tests[MAX_TESTS];
extern int g_test_count;

struct TestRegistrar {
    TestRegistrar(const char *name, void (*func)(), const char *file,
                  int line) {
        if (g_test_count >= MAX_TESTS) {
            printf("ERROR: Too many tests! Increase MAX_TESTS (currently %d)\n", MAX_TESTS);
            printf("Failed to register: %s\n", name);
            return;
        }
        g_tests[g_test_count++] = {name, func, file, line};
    }
};

// Current test state
struct TestState {
    int passed = 0;
    int failed = 0;
    const char *current_test = nullptr;
    bool current_failed = false;
};

extern TestState g_state;

// Assertion macros
#define ASSERT(expr)                                                     \
    do {                                                                 \
        if (!(expr)) {                                                   \
            printf("FAILED\n    Assertion failed: %s at %s:%d\n", #expr, \
                   __FILE__, __LINE__);                                  \
            g_state.current_failed = true;                               \
            return;                                                      \
        }                                                                \
    } while (0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_TRUE(expr) ASSERT(expr)
#define ASSERT_FALSE(expr) ASSERT(!(expr))
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0)

// Test registration macro
#define TEST(category, name)                                                \
    static void test_##category##_##name();                                 \
    static TestRegistrar registrar_##category##_##name(                     \
        #category "/" #name, test_##category##_##name, __FILE__, __LINE__); \
    static void test_##category##_##name()

// Main runner - implemented in main.cpp
int run_all_tests();

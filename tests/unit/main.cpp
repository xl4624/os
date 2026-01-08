#include "../framework/test.hpp"

TestCase g_tests[MAX_TESTS];
int g_test_count = 0;
TestState g_state;

int main() {
    if (g_test_count == 0) {
        printf("No tests found!\n");
        return 1;
    }

    printf("myos test suite\n");
    printf("===============\n\n");

    // Group by category
    char current_category[64] = "";
    for (int i = 0; i < g_test_count; i++) {
        TestCase &test = g_tests[i];
        const char *slash = strchr(test.name, '/');
        if (!slash)
            continue;

        size_t cat_len = slash - test.name;
        char category[64];
        for (size_t j = 0; j < cat_len && j < 63; j++)
            category[j] = test.name[j];
        category[cat_len] = '\0';
        const char *test_name = slash + 1;

        if (strcmp(category, current_category) != 0) {
            if (current_category[0]) {
                printf("\n");
            }
            strcpy(current_category, category);
            printf("[%s]\n", category);
        }

        printf("  %s ... ", test_name);

        g_state.current_test = test.name;
        g_state.current_failed = false;
        test.func();

        if (!g_state.current_failed) {
            printf("OK\n");
            g_state.passed++;
        } else {
            g_state.failed++;
        }
    }

    printf("\n========================================\n");
    printf("Total: %d passed, %d failed\n", g_state.passed, g_state.failed);
    printf("========================================\n");

    return g_state.failed > 0 ? 1 : 0;
}

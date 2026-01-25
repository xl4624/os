#include "../framework/test.hpp"

TestCase kTests[kMaxTests];
int kTestCount = 0;
TestState kTestState;

int main() {
    if (kTestCount == 0) {
        printf("No tests found!\n");
        return 1;
    }

    // Group by category
    char current_category[64] = "";
    for (int i = 0; i < kTestCount; i++) {
        TestCase &test = kTests[i];
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

        kTestState.current_test = test.name;
        kTestState.current_failed = false;
        test.func();

        if (!kTestState.current_failed) {
            printf("OK\n");
            kTestState.passed++;
        } else {
            kTestState.failed++;
        }
    }

    printf("\n========================================\n");
    printf("Total: %d passed, %d failed\n", kTestState.passed,
           kTestState.failed);
    printf("========================================\n");

    return kTestState.failed > 0 ? 1 : 0;
}

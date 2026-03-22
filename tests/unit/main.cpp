#include "../framework/test.h"

TestState kTestState;

int main() {
  auto& tests = get_tests();

  if (tests.empty()) {
    printf("No tests found!\n");
    return 1;
  }

  // Group by category
  char current_category[64] = "";
  for (const auto& test : tests) {
    const char* slash = strchr(test.name, '/');
    if (slash == nullptr) {
      continue;
    }

    const size_t cat_len = slash - test.name;
    char category[64];
    for (size_t j = 0; j < cat_len && j < 63; ++j) {
      category[j] = test.name[j];
    }
    category[cat_len] = '\0';
    const char* test_name = slash + 1;

    if (strcmp(category, current_category) != 0) {
      if (current_category[0] != 0) {
        printf("\n");
      }
      strncpy(current_category, category, sizeof(current_category) - 1);
      current_category[sizeof(current_category) - 1] = '\0';
      printf("[%s]\n", category);
    }

    printf("  %s ... ", test_name);

    kTestState.current_test = test.name;
    kTestState.current_failed = false;
    test.func();

    if (!kTestState.current_failed) {
      printf("OK\n");
      ++kTestState.passed;
    } else {
      ++kTestState.failed;
    }
  }

  printf("\n========================================\n");
  printf("Total: %d passed, %d failed\n", kTestState.passed, kTestState.failed);
  printf("========================================\n");

  return kTestState.failed == 0 ? 0 : 1;
}

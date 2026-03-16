#include <stdio.h>

#include "../framework/test.h"

TEST(printf, fprintf_empty_format) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"
  const int n = fprintf(stdout, "");
#pragma GCC diagnostic pop
  ASSERT_EQ(n, 0);
}

TEST(printf, fprintf_plain_string) {
  // "hello\n" = 6 chars
  const int n = fprintf(stdout, "hello\n");
  ASSERT_EQ(n, 6);
}

TEST(printf, fprintf_matches_snprintf_count) {
  // Both snprintf and fprintf use the same vformat core, so they must return
  // the same character count for identical format strings and arguments.
  const char* fmt = "val: %d hex: %x str: %s";
  char buf[64];
  const int n_snprintf = snprintf(buf, sizeof(buf), fmt, 42, 0xab, "world");
  const int n_fprintf = fprintf(stdout, fmt, 42, 0xab, "world");
  fprintf(stdout, "\n");
  ASSERT_EQ(n_snprintf, n_fprintf);
}

TEST(printf, fprintf_stderr) {
  // fprintf to stderr should work the same way as stdout
  // "test\n" = 5 chars
  const int n = fprintf(stderr, "test\n");
  ASSERT_EQ(n, 5);
}

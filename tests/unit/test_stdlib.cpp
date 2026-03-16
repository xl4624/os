#include <stdlib.h>

#include "../framework/test.h"

TEST(stdlib, abs_positive) {
  ASSERT(abs(5) == 5);
  ASSERT(abs(1) == 1);
  ASSERT(abs(12345) == 12345);
}

TEST(stdlib, abs_negative) {
  ASSERT(abs(-5) == 5);
  ASSERT(abs(-1) == 1);
  ASSERT(abs(-12345) == 12345);
}

TEST(stdlib, abs_zero) { ASSERT(abs(0) == 0); }

TEST(stdlib, itoa_positive) {
  char buf[20];
  itoa(123, buf, 10);
  ASSERT_STR_EQ(buf, "123");
}

TEST(stdlib, itoa_negative) {
  char buf[20];
  itoa(-456, buf, 10);
  ASSERT_STR_EQ(buf, "-456");
}

TEST(stdlib, itoa_hex) {
  char buf[20];
  itoa(255, buf, 16);
  ASSERT_STR_EQ(buf, "ff");
}

TEST(stdlib, itoa_zero) {
  char buf[20];
  itoa(0, buf, 10);
  ASSERT_STR_EQ(buf, "0");
}

TEST(stdlib, itoa_binary) {
  char buf[20];
  itoa(10, buf, 2);
  ASSERT_STR_EQ(buf, "1010");
  itoa(255, buf, 2);
  ASSERT_STR_EQ(buf, "11111111");
}

TEST(stdlib, itoa_octal) {
  char buf[20];
  itoa(64, buf, 8);
  ASSERT_STR_EQ(buf, "100");
  itoa(255, buf, 8);
  ASSERT_STR_EQ(buf, "377");
}

TEST(stdlib, itoa_large_positive) {
  char buf[20];
  itoa(2147483647, buf, 10);
  ASSERT_STR_EQ(buf, "2147483647");
}

TEST(stdlib, itoa_large_negative) {
  char buf[20];
  itoa(-2147483648, buf, 10);
  ASSERT_STR_EQ(buf, "-2147483648");
}

TEST(stdlib, itoa_single_digit) {
  char buf[20];
  itoa(5, buf, 10);
  ASSERT_STR_EQ(buf, "5");
  itoa(-7, buf, 10);
  ASSERT_STR_EQ(buf, "-7");
}

TEST(stdlib, itoa_hex_uppercase_not_used) {
  char buf[20];
  itoa(255, buf, 16);
  ASSERT_STR_EQ(buf, "ff");
  itoa(10, buf, 16);
  ASSERT_STR_EQ(buf, "a");
}

TEST(stdlib, itoa_base_36) {
  char buf[20];
  itoa(35, buf, 36);
  ASSERT_STR_EQ(buf, "z");
}

// Negative values in non-decimal bases get a '-' prefix with the magnitude
// converted in the given base.
TEST(stdlib, itoa_negative_in_hex) {
  char buf[20];
  itoa(-255, buf, 16);
  ASSERT_STR_EQ(buf, "-ff");
  itoa(-1, buf, 16);
  ASSERT_STR_EQ(buf, "-1");
}

TEST(stdlib, itoa_negative_in_binary) {
  char buf[20];
  itoa(-10, buf, 2);
  ASSERT_STR_EQ(buf, "-1010");
}

TEST(stdlib, itoa_all_hex_letters) {
  char buf[20];
  itoa(10, buf, 16);
  ASSERT_STR_EQ(buf, "a");
  itoa(11, buf, 16);
  ASSERT_STR_EQ(buf, "b");
  itoa(12, buf, 16);
  ASSERT_STR_EQ(buf, "c");
  itoa(13, buf, 16);
  ASSERT_STR_EQ(buf, "d");
  itoa(14, buf, 16);
  ASSERT_STR_EQ(buf, "e");
  itoa(15, buf, 16);
  ASSERT_STR_EQ(buf, "f");
}

TEST(stdlib, abs_int_max) { ASSERT_EQ(abs(2147483647), 2147483647); }

TEST(stdlib, atoi_basic) { ASSERT(atoi("123") == 123); }

TEST(stdlib, atoi_negative) { ASSERT(atoi("-456") == -456); }

TEST(stdlib, atoi_zero) { ASSERT(atoi("0") == 0); }

TEST(stdlib, atoi_with_spaces) { ASSERT(atoi("  789") == 789); }

TEST(stdlib, atoi_invalid) { ASSERT(atoi("abc") == 0); }

TEST(stdlib, strtol_basic) { ASSERT(strtol("123", nullptr, 10) == 123); }

TEST(stdlib, strtol_negative) { ASSERT(strtol("-456", nullptr, 10) == -456); }

TEST(stdlib, strtol_hex) { ASSERT(strtol("ff", nullptr, 16) == 255); }

TEST(stdlib, strtol_hex_prefix) { ASSERT(strtol("0xff", nullptr, 0) == 255); }

TEST(stdlib, strtol_octal_prefix) { ASSERT(strtol("0777", nullptr, 0) == 0777); }

TEST(stdlib, strtol_base_0_auto) {
  ASSERT(strtol("0x10", nullptr, 0) == 16);
  ASSERT(strtol("010", nullptr, 0) == 8);
  ASSERT(strtol("10", nullptr, 0) == 10);
}

TEST(stdlib, strtol_endptr) {
  const char* s = "123abc";
  char* end;
  ASSERT(strtol(s, &end, 10) == 123);
  ASSERT(end == s + 3);
}

TEST(stdlib, strtoul_basic) { ASSERT(strtoul("123", nullptr, 10) == 123); }

TEST(stdlib, strtoul_hex) { ASSERT(strtoul("ff", nullptr, 16) == 255); }

TEST(stdlib, snprintf_basic) {
  char buf[32];
  const int n = snprintf(buf, sizeof(buf), "hello");
  ASSERT_EQ(n, 5);
  ASSERT_STR_EQ(buf, "hello");
}

TEST(stdlib, snprintf_truncation) {
  char buf[32];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
  const int n = snprintf(buf, 5, "hello world");
#pragma GCC diagnostic pop
  ASSERT_EQ(n, 11);  // total chars that would be written
  ASSERT_STR_EQ(buf, "hell");
}

TEST(stdlib, snprintf_zero_size) {
  char buf[32];
  const int n = snprintf(buf, 0, "test");
  ASSERT_EQ(n, 4);  // total chars that would be written
}

TEST(stdlib, snprintf_format) {
  char buf[32];
  snprintf(buf, sizeof(buf), "value: %d", 42);
  ASSERT_STR_EQ(buf, "value: 42");
}

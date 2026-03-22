// NOLINTBEGIN(bugprone-unchecked-string-to-number-conversion)
#include <stdio.h>
#include <string.h>

#include "../framework/test.h"

TEST(sscanf, decimal) {
  int val;
  ASSERT(sscanf("123", "%d", &val) == 1);
  ASSERT(val == 123);
}

TEST(sscanf, decimal_negative) {
  int val;
  ASSERT(sscanf("-456", "%d", &val) == 1);
  ASSERT(val == -456);
}

TEST(sscanf, hex) {
  unsigned int val;
  ASSERT(sscanf("ff", "%x", &val) == 1);
  ASSERT(val == 255);
}

TEST(sscanf, hex_with_prefix) {
  unsigned int val;
  ASSERT(sscanf("0xdead", "%x", &val) == 1);
  ASSERT(val == 0xdead);
}

TEST(sscanf, string) {
  char buf[32];
  ASSERT(sscanf("hello", "%s", buf) == 1);
  ASSERT(strcmp(buf, "hello") == 0);
}

TEST(sscanf, character) {
  char c;
  ASSERT(sscanf("x", "%c", &c) == 1);
  ASSERT(c == 'x');
}

TEST(sscanf, mixed) {
  int num;
  char str[32];
  ASSERT(sscanf("42 hello", "%d %s", &num, str) == 2);
  ASSERT(num == 42);
  ASSERT(strcmp(str, "hello") == 0);
}
// NOLINTEND(bugprone-unchecked-string-to-number-conversion)

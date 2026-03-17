#include <algorithm.h>

#include "../framework/test.h"

TEST(algorithm, min_int) {
  const int a = 1;
  const int b = 2;
  ASSERT(std::min(a, b) == 1);
}

TEST(algorithm, min_equal) {
  int x = 5;
  ASSERT(std::min(x, x) == 5);
}

TEST(algorithm, max_int) {
  const int a = 1;
  const int b = 2;
  ASSERT(std::max(a, b) == 2);
}

TEST(algorithm, max_equal) {
  int x = 5;
  ASSERT(std::max(x, x) == 5);
}

TEST(algorithm, min_double) {
  const double a = 1.5;
  const double b = 2.5;
  ASSERT(std::min(a, b) == 1.5);
}

TEST(algorithm, max_double) {
  const double a = 1.5;
  const double b = 2.5;
  ASSERT(std::max(a, b) == 2.5);
}

TEST(algorithm, min_negative) {
  const int a = -5;
  const int b = 3;
  ASSERT(std::min(a, b) == -5);
}

TEST(algorithm, max_negative) {
  const int a = -5;
  const int b = 3;
  ASSERT(std::max(a, b) == 3);
}

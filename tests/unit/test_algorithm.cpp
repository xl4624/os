#include <algorithm.h>

#include "../framework/test.h"

TEST(algorithm, min_int) { ASSERT(std::min(1, 2) == 1); }

TEST(algorithm, min_equal) {
  int x = 5;
  ASSERT(std::min(x, x) == 5);
}

TEST(algorithm, max_int) { ASSERT(std::max(1, 2) == 2); }

TEST(algorithm, max_equal) {
  int x = 5;
  ASSERT(std::max(x, x) == 5);
}

TEST(algorithm, min_double) { ASSERT(std::min(1.5, 2.5) == 1.5); }

TEST(algorithm, max_double) { ASSERT(std::max(1.5, 2.5) == 2.5); }

TEST(algorithm, min_negative) { ASSERT(std::min(-5, 3) == -5); }

TEST(algorithm, max_negative) { ASSERT(std::max(-5, 3) == 3); }

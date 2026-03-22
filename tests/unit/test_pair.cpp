#include <utility.h>

#include "../framework/test.h"

// ===========================================================================
// Construction
// ===========================================================================

TEST(pair, default_construct) {
  std::pair<int, int> p;
  ASSERT_EQ(p.first, 0);
  ASSERT_EQ(p.second, 0);
}

TEST(pair, value_construct) {
  std::pair<int, float> p(42, 3.14F);
  ASSERT_EQ(p.first, 42);
  ASSERT_TRUE(p.second > 3.0F);
}

TEST(pair, copy_construct) {
  std::pair<int, int> a(1, 2);
  std::pair<int, int> b(a);
  ASSERT_EQ(b.first, 1);
  ASSERT_EQ(b.second, 2);
}

TEST(pair, move_construct) {
  std::pair<int, int> a(10, 20);
  std::pair<int, int> b(std::move(a));
  ASSERT_EQ(b.first, 10);
  ASSERT_EQ(b.second, 20);
}

// ===========================================================================
// Assignment
// ===========================================================================

TEST(pair, copy_assign) {
  std::pair<int, int> a(5, 6);
  std::pair<int, int> b;
  b = a;
  ASSERT_EQ(b.first, 5);
  ASSERT_EQ(b.second, 6);
}

TEST(pair, move_assign) {
  std::pair<int, int> a(7, 8);
  std::pair<int, int> b;
  b = std::move(a);
  ASSERT_EQ(b.first, 7);
  ASSERT_EQ(b.second, 8);
}

// ===========================================================================
// make_pair
// ===========================================================================

TEST(pair, make_pair) {
  auto p = std::make_pair(100, 'A');
  static_assert(std::is_same_v<decltype(p.first), int>);
  static_assert(std::is_same_v<decltype(p.second), char>);
  ASSERT_EQ(p.first, 100);
  ASSERT_EQ(p.second, 'A');
}

// ===========================================================================
// Comparison
// ===========================================================================

TEST(pair, equality) {
  std::pair<int, int> a(1, 2);
  std::pair<int, int> b(1, 2);
  std::pair<int, int> c(1, 3);
  ASSERT_TRUE(a == b);
  ASSERT_TRUE(a != c);
}

TEST(pair, less_than_first_differs) {
  std::pair<int, int> a(1, 9);
  std::pair<int, int> b(2, 0);
  ASSERT_TRUE(a < b);
  ASSERT_FALSE(b < a);
}

TEST(pair, less_than_first_equal) {
  std::pair<int, int> a(1, 2);
  std::pair<int, int> b(1, 3);
  ASSERT_TRUE(a < b);
  ASSERT_FALSE(b < a);
}

TEST(pair, less_than_equal) {
  std::pair<int, int> a(1, 2);
  std::pair<int, int> b(1, 2);
  ASSERT_TRUE(a <= b);
  ASSERT_TRUE(a >= b);
  ASSERT_FALSE(a < b);
  ASSERT_FALSE(a > b);
}

// ===========================================================================
// swap
// ===========================================================================

TEST(pair, swap) {
  std::pair<int, int> a(1, 2);
  std::pair<int, int> b(3, 4);
  a.swap(b);
  ASSERT_EQ(a.first, 3);
  ASSERT_EQ(a.second, 4);
  ASSERT_EQ(b.first, 1);
  ASSERT_EQ(b.second, 2);
}

// ===========================================================================
// Deduction guide
// ===========================================================================

TEST(pair, deduction_guide) {
  std::pair p(42, 3.14);
  static_assert(std::is_same_v<decltype(p.first), int>);
  static_assert(std::is_same_v<decltype(p.second), double>);
  ASSERT_EQ(p.first, 42);
}

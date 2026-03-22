#include <tuple.h>

#include "../framework/test.h"

// ===========================================================================
// Construction
// ===========================================================================

TEST(tuple, default_construct) {
  std::tuple<int, float, char> t;
  ASSERT_EQ(std::get<0>(t), 0);
  ASSERT_EQ(std::get<2>(t), '\0');
}

TEST(tuple, value_construct) {
  std::tuple<int, double, char> t(42, 3.14, 'A');
  ASSERT_EQ(std::get<0>(t), 42);
  ASSERT_EQ(std::get<2>(t), 'A');
}

TEST(tuple, copy_construct) {
  std::tuple<int, int> a(1, 2);
  std::tuple<int, int> b(a);
  ASSERT_EQ(std::get<0>(b), 1);
  ASSERT_EQ(std::get<1>(b), 2);
}

TEST(tuple, move_construct) {
  std::tuple<int, int> a(10, 20);
  std::tuple<int, int> b(std::move(a));
  ASSERT_EQ(std::get<0>(b), 10);
  ASSERT_EQ(std::get<1>(b), 20);
}

// ===========================================================================
// std::get
// ===========================================================================

TEST(tuple, get_mutable) {
  std::tuple<int, int> t(1, 2);
  std::get<0>(t) = 99;
  ASSERT_EQ(std::get<0>(t), 99);
  ASSERT_EQ(std::get<1>(t), 2);
}

TEST(tuple, get_const) {
  const std::tuple<int, char> t(42, 'Z');
  ASSERT_EQ(std::get<0>(t), 42);
  ASSERT_EQ(std::get<1>(t), 'Z');
}

// ===========================================================================
// make_tuple
// ===========================================================================

TEST(tuple, make_tuple) {
  auto t = std::make_tuple(1, 2.0, 'c');
  static_assert(std::is_same_v<decltype(std::get<0>(t)), int&>);
  static_assert(std::is_same_v<decltype(std::get<1>(t)), double&>);
  static_assert(std::is_same_v<decltype(std::get<2>(t)), char&>);
  ASSERT_EQ(std::get<0>(t), 1);
  ASSERT_EQ(std::get<2>(t), 'c');
}

// ===========================================================================
// tie
// ===========================================================================

TEST(tuple, tie) {
  int a = 0;
  int b = 0;
  auto t = std::tie(a, b);
  std::get<0>(t) = 10;
  std::get<1>(t) = 20;
  ASSERT_EQ(a, 10);
  ASSERT_EQ(b, 20);
}

// ===========================================================================
// tuple_size
// ===========================================================================

TEST(tuple, tuple_size) {
  static_assert(std::tuple_size_v<std::tuple<int, float, char>> == 3);
  static_assert(std::tuple_size_v<std::tuple<>> == 0);
  static_assert(std::tuple_size_v<std::tuple<int>> == 1);
  using IntPair = std::tuple<int, int>;
  ASSERT_EQ(std::tuple_size_v<IntPair>, 2U);
}

// ===========================================================================
// tuple_element
// ===========================================================================

TEST(tuple, tuple_element) {
  static_assert(std::is_same_v<std::tuple_element_t<0, std::tuple<int, float>>, int>);
  static_assert(std::is_same_v<std::tuple_element_t<1, std::tuple<int, float>>, float>);
  ASSERT_TRUE((std::is_same_v<std::tuple_element_t<0, std::tuple<char, int>>, char>));
}

// ===========================================================================
// Comparison
// ===========================================================================

TEST(tuple, equality) {
  auto a = std::make_tuple(1, 2, 3);
  auto b = std::make_tuple(1, 2, 3);
  auto c = std::make_tuple(1, 2, 4);
  ASSERT_TRUE(a == b);
  ASSERT_TRUE(a != c);
}

TEST(tuple, less_than) {
  auto a = std::make_tuple(1, 2, 3);
  auto b = std::make_tuple(1, 2, 4);
  auto c = std::make_tuple(2, 0, 0);
  ASSERT_TRUE(a < b);
  ASSERT_FALSE(b < a);
  ASSERT_TRUE(a < c);
}

// ===========================================================================
// Deduction guide
// ===========================================================================

TEST(tuple, deduction_guide) {
  std::tuple t(1, 2.0, 'a');
  static_assert(std::tuple_size_v<decltype(t)> == 3);
  ASSERT_EQ(std::get<0>(t), 1);
}

// ===========================================================================
// Single and empty tuples
// ===========================================================================

TEST(tuple, single_element) {
  std::tuple<int> t(42);
  ASSERT_EQ(std::get<0>(t), 42);
}

TEST(tuple, empty_tuple) {
  std::tuple<> t;
  static_assert(std::tuple_size_v<decltype(t)> == 0);
  ASSERT_EQ(std::tuple_size_v<decltype(t)>, 0U);
}

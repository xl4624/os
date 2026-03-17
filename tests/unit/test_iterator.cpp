#include <array.h>
#include <iterator.h>

#include "../framework/test.h"

// ===========================================================================
// iterator_traits
// ===========================================================================

TEST(iterator_traits, pointer_category) {
  using traits = std::iterator_traits<int*>;
  static_assert(std::is_same_v<traits::iterator_category, std::random_access_iterator_tag>,
                "category");
  static_assert(std::is_same_v<traits::value_type, int>, "value_type");
  static_assert(std::is_same_v<traits::pointer, int*>, "pointer");
  static_assert(std::is_same_v<traits::reference, int&>, "reference");
  ASSERT(true);
}

TEST(iterator_traits, const_pointer_category) {
  using traits = std::iterator_traits<const int*>;
  static_assert(std::is_same_v<traits::iterator_category, std::random_access_iterator_tag>,
                "category");
  static_assert(std::is_same_v<traits::value_type, int>, "value_type");
  static_assert(std::is_same_v<traits::pointer, const int*>, "pointer");
  static_assert(std::is_same_v<traits::reference, const int&>, "reference");
  ASSERT(true);
}

// ===========================================================================
// reverse_iterator
// ===========================================================================

TEST(reverse_iterator, basic_traversal) {
  std::array<int, 4> a = {1, 2, 3, 4};
  int expected[] = {4, 3, 2, 1};
  int i = 0;
  for (auto it = a.rbegin(); it != a.rend(); ++it) {
    ASSERT_EQ(*it, expected[i++]);
  }
}

TEST(reverse_iterator, base_roundtrip) {
  std::array<int, 3> a = {10, 20, 30};
  // rbegin().base() == end()
  ASSERT(a.rbegin().base() == a.end());
  // rend().base() == begin()
  ASSERT(a.rend().base() == a.begin());
}

TEST(reverse_iterator, pre_decrement) {
  std::array<int, 3> a = {1, 2, 3};
  auto it = a.rend();
  --it;
  ASSERT_EQ(*it, 1);
}

TEST(reverse_iterator, post_increment) {
  std::array<int, 3> a = {7, 8, 9};
  auto it = a.rbegin();
  auto old = it++;
  ASSERT_EQ(*old, 9);
  ASSERT_EQ(*it, 8);
}

TEST(reverse_iterator, arithmetic) {
  std::array<int, 5> a = {0, 1, 2, 3, 4};
  auto it = a.rbegin();
  ASSERT_EQ(*(it + 2), 2);
  it += 3;
  ASSERT_EQ(*it, 1);
  it -= 1;
  ASSERT_EQ(*it, 2);
}

TEST(reverse_iterator, subscript) {
  std::array<int, 5> a = {10, 20, 30, 40, 50};
  auto it = a.rbegin();
  ASSERT_EQ(it[0], 50);
  ASSERT_EQ(it[4], 10);
}

TEST(reverse_iterator, distance) {
  std::array<int, 4> a = {1, 2, 3, 4};
  auto diff = a.rend() - a.rbegin();
  ASSERT_EQ(diff, 4);
}

TEST(reverse_iterator, comparisons) {
  std::array<int, 3> a = {1, 2, 3};
  auto first = a.rbegin();
  auto last = a.rend();
  ASSERT(first < last);
  ASSERT(last > first);
  ASSERT(first <= first);
  ASSERT(first >= first);
  ASSERT(first != last);
  ASSERT(first == a.rbegin());
}

TEST(reverse_iterator, const_reverse_iterator) {
  const std::array<int, 3> a = {5, 6, 7};
  int expected[] = {7, 6, 5};
  int i = 0;
  for (auto it = a.rbegin(); it != a.rend(); ++it) {
    ASSERT_EQ(*it, expected[i++]);
  }
}

TEST(reverse_iterator, crbegin_crend) {
  std::array<int, 4> a = {1, 2, 3, 4};
  int sum = 0;
  for (auto it = a.crbegin(); it != a.crend(); ++it) {
    sum += *it;
  }
  ASSERT_EQ(sum, 10);
}

// ===========================================================================
// begin() / end() free functions
// ===========================================================================

TEST(iterator_free, c_array_begin_end) {
  int arr[4] = {1, 2, 3, 4};
  int sum = 0;
  for (int* it = std::begin(arr); it != std::end(arr); ++it) {
    sum += *it;
  }
  ASSERT_EQ(sum, 10);
}

TEST(iterator_free, c_array_rbegin_rend) {
  int arr[3] = {10, 20, 30};
  int expected[] = {30, 20, 10};
  int i = 0;
  for (auto it = std::rbegin(arr); it != std::rend(arr); ++it) {
    ASSERT_EQ(*it, expected[i++]);
  }
}

TEST(iterator_free, container_begin_end) {
  std::array<int, 3> a = {4, 5, 6};
  int sum = 0;
  for (auto it = std::begin(a); it != std::end(a); ++it) {
    sum += *it;
  }
  ASSERT_EQ(sum, 15);
}

TEST(iterator_free, cbegin_cend) {
  const std::array<int, 3> a = {1, 2, 3};
  int sum = 0;
  for (auto it = std::cbegin(a); it != std::cend(a); ++it) {
    sum += *it;
  }
  ASSERT_EQ(sum, 6);
}

TEST(iterator_free, crbegin_crend) {
  const std::array<int, 3> a = {1, 2, 3};
  int vals[3];
  int i = 0;
  for (auto it = std::crbegin(a); it != std::crend(a); ++it) {
    vals[i++] = *it;
  }
  ASSERT_EQ(vals[0], 3);
  ASSERT_EQ(vals[1], 2);
  ASSERT_EQ(vals[2], 1);
}

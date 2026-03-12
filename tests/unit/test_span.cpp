#include <array.h>
#include <span.h>

#include "../framework/test.h"

// ===========================================================================
// Construction
// ===========================================================================

TEST(span, default_is_empty) {
  std::span<int> s;
  ASSERT_EQ(s.size(), 0u);
  ASSERT_TRUE(s.empty());
  ASSERT_NULL(s.data());
}

TEST(span, ptr_count_construction) {
  int arr[4] = {1, 2, 3, 4};
  std::span<int> s(arr, 4);
  ASSERT_EQ(s.size(), 4u);
  ASSERT_FALSE(s.empty());
  ASSERT_EQ(s.data(), arr);
}

TEST(span, ptr_range_construction) {
  int arr[3] = {10, 20, 30};
  std::span<int> s(arr, arr + 3);
  ASSERT_EQ(s.size(), 3u);
  ASSERT_EQ(s[0], 10);
  ASSERT_EQ(s[2], 30);
}

TEST(span, c_array_construction) {
  int arr[5] = {1, 2, 3, 4, 5};
  std::span<int> s(arr);
  ASSERT_EQ(s.size(), 5u);
  ASSERT_EQ(s.data(), arr);
}

TEST(span, std_array_construction) {
  std::array<int, 3> arr = {7, 8, 9};
  std::span<int> s(arr);
  ASSERT_EQ(s.size(), 3u);
  ASSERT_EQ(s[0], 7);
  ASSERT_EQ(s[2], 9);
}

TEST(span, const_std_array_construction) {
  const std::array<int, 2> arr = {4, 5};
  std::span<const int> s(arr);
  ASSERT_EQ(s.size(), 2u);
  ASSERT_EQ(s[0], 4);
  ASSERT_EQ(s[1], 5);
}

TEST(span, copy_construction) {
  int arr[3] = {1, 2, 3};
  std::span<int> a(arr, 3);
  std::span<int> b(a);
  ASSERT_EQ(b.size(), 3u);
  ASSERT_EQ(b.data(), arr);
}

// ===========================================================================
// Element access
// ===========================================================================

TEST(span, subscript_operator) {
  int arr[4] = {10, 20, 30, 40};
  std::span<int> s(arr);
  ASSERT_EQ(s[0], 10);
  ASSERT_EQ(s[3], 40);
}

TEST(span, subscript_write) {
  int arr[3] = {1, 2, 3};
  std::span<int> s(arr);
  s[1] = 99;
  ASSERT_EQ(arr[1], 99);
}

TEST(span, front_and_back) {
  int arr[4] = {5, 6, 7, 8};
  std::span<int> s(arr);
  ASSERT_EQ(s.front(), 5);
  ASSERT_EQ(s.back(), 8);
}

TEST(span, data_points_to_first_element) {
  int arr[3] = {1, 2, 3};
  std::span<int> s(arr);
  ASSERT(s.data() == &arr[0]);
}

// ===========================================================================
// Observers
// ===========================================================================

TEST(span, size_and_empty) {
  int arr[4] = {};
  std::span<int> s(arr);
  ASSERT_EQ(s.size(), 4u);
  ASSERT_FALSE(s.empty());
}

TEST(span, size_bytes) {
  int arr[3] = {};
  std::span<int> s(arr);
  ASSERT_EQ(s.size_bytes(), 3u * sizeof(int));
}

// ===========================================================================
// Iterators
// ===========================================================================

TEST(span, iterator_loop) {
  int arr[5] = {1, 2, 3, 4, 5};
  std::span<int> s(arr);
  int sum = 0;
  for (auto it = s.begin(); it != s.end(); ++it) {
    sum += *it;
  }
  ASSERT_EQ(sum, 15);
}

TEST(span, range_based_for) {
  int arr[4] = {1, 2, 3, 4};
  std::span<int> s(arr);
  int sum = 0;
  for (int v : s) sum += v;
  ASSERT_EQ(sum, 10);
}

TEST(span, const_iterators) {
  int arr[3] = {1, 2, 3};
  std::span<int> s(arr);
  int sum = 0;
  for (auto it = s.cbegin(); it != s.cend(); ++it) sum += *it;
  ASSERT_EQ(sum, 6);
}

// ===========================================================================
// Subspans
// ===========================================================================

TEST(span, first_n_elements) {
  int arr[5] = {1, 2, 3, 4, 5};
  std::span<int> s(arr);
  auto f = s.first(3);
  ASSERT_EQ(f.size(), 3u);
  ASSERT_EQ(f[0], 1);
  ASSERT_EQ(f[2], 3);
}

TEST(span, last_n_elements) {
  int arr[5] = {1, 2, 3, 4, 5};
  std::span<int> s(arr);
  auto l = s.last(2);
  ASSERT_EQ(l.size(), 2u);
  ASSERT_EQ(l[0], 4);
  ASSERT_EQ(l[1], 5);
}

TEST(span, subspan_with_offset) {
  int arr[6] = {1, 2, 3, 4, 5, 6};
  std::span<int> s(arr);
  auto sub = s.subspan(2);
  ASSERT_EQ(sub.size(), 4u);
  ASSERT_EQ(sub[0], 3);
  ASSERT_EQ(sub[3], 6);
}

TEST(span, subspan_with_offset_and_count) {
  int arr[6] = {1, 2, 3, 4, 5, 6};
  std::span<int> s(arr);
  auto sub = s.subspan(1, 3);
  ASSERT_EQ(sub.size(), 3u);
  ASSERT_EQ(sub[0], 2);
  ASSERT_EQ(sub[2], 4);
}

TEST(span, subspan_zero_offset_full_length) {
  int arr[4] = {1, 2, 3, 4};
  std::span<int> s(arr);
  auto sub = s.subspan(0);
  ASSERT_EQ(sub.size(), 4u);
  ASSERT_EQ(sub[0], 1);
}

// ===========================================================================
// Const span
// ===========================================================================

TEST(span, const_span_element_read) {
  int arr[3] = {10, 20, 30};
  const std::span<int> s(arr, 3);
  ASSERT_EQ(s[0], 10);
  ASSERT_EQ(s.front(), 10);
  ASSERT_EQ(s.back(), 30);
}

TEST(span, span_of_const_int) {
  int arr[3] = {5, 6, 7};
  std::span<const int> s(arr);
  ASSERT_EQ(s[1], 6);
  int sum = 0;
  for (int v : s) sum += v;
  ASSERT_EQ(sum, 18);
}

TEST(span, writes_propagate_to_underlying_array) {
  int arr[3] = {0, 0, 0};
  std::span<int> s(arr);
  s[0] = 1;
  s[1] = 2;
  s[2] = 3;
  ASSERT_EQ(arr[0], 1);
  ASSERT_EQ(arr[1], 2);
  ASSERT_EQ(arr[2], 3);
}

TEST(span, copy_assignment) {
  int a[3] = {1, 2, 3};
  int b[2] = {4, 5};
  std::span<int> sa(a);
  std::span<int> sb(b);
  sa = sb;
  ASSERT_EQ(sa.size(), 2u);
  ASSERT_EQ(sa.data(), b);
}

TEST(span, first_zero_is_empty) {
  int arr[4] = {1, 2, 3, 4};
  std::span<int> s(arr);
  auto f = s.first(0);
  ASSERT_EQ(f.size(), 0u);
  ASSERT_TRUE(f.empty());
}

TEST(span, last_zero_is_empty) {
  int arr[4] = {1, 2, 3, 4};
  std::span<int> s(arr);
  auto l = s.last(0);
  ASSERT_EQ(l.size(), 0u);
  ASSERT_TRUE(l.empty());
}

TEST(span, subspan_zero_count_is_empty) {
  int arr[5] = {1, 2, 3, 4, 5};
  std::span<int> s(arr);
  auto sub = s.subspan(2, 0);
  ASSERT_EQ(sub.size(), 0u);
  ASSERT_TRUE(sub.empty());
}

// ===========================================================================
// Char span
// ===========================================================================

TEST(span, char_span_from_array) {
  char buf[6] = {'h', 'e', 'l', 'l', 'o', '\0'};
  std::span<char> s(buf);
  ASSERT_EQ(s.size(), 6u);
  ASSERT_EQ(s[0], 'h');
  ASSERT_EQ(s[4], 'o');
}

TEST(span, const_char_span_read) {
  const char msg[] = "abc";
  std::span<const char> s(msg, 3);
  ASSERT_EQ(s.size(), 3u);
  ASSERT_EQ(s[0], 'a');
  ASSERT_EQ(s[2], 'c');
}

TEST(span, char_span_subspan) {
  char buf[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
  std::span<char> s(buf);
  auto mid = s.subspan(2, 4);
  ASSERT_EQ(mid.size(), 4u);
  ASSERT_EQ(mid[0], 'c');
  ASSERT_EQ(mid[3], 'f');
}

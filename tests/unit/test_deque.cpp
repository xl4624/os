// NOLINTBEGIN(misc-const-correctness)
#include <deque.h>

#include "../framework/test.h"

// ===========================================================================
// Construction
// ===========================================================================

TEST(deque, default_construct_is_empty) {
  std::deque<int> d;
  ASSERT_TRUE(d.empty());
  ASSERT_EQ(d.size(), 0U);
}

TEST(deque, size_construct) {
  std::deque<int> d(5);
  ASSERT_EQ(d.size(), 5U);
  for (size_t i = 0; i < 5; ++i) {
    ASSERT_EQ(d[i], 0);
  }
}

TEST(deque, fill_construct) {
  std::deque<int> d(4, 7);
  ASSERT_EQ(d.size(), 4U);
  for (size_t i = 0; i < 4; ++i) {
    ASSERT_EQ(d[i], 7);
  }
}

TEST(deque, initializer_list_construct) {
  std::deque<int> d = {1, 2, 3, 4};
  ASSERT_EQ(d.size(), 4U);
  ASSERT_EQ(d[0], 1);
  ASSERT_EQ(d[3], 4);
}

TEST(deque, copy_construct) {
  std::deque<int> a = {10, 20, 30};
  std::deque<int> b(a);
  ASSERT_EQ(b.size(), 3U);
  ASSERT_EQ(b[0], 10);
  ASSERT_EQ(b[2], 30);
  b[0] = 99;
  ASSERT_EQ(a[0], 10);
}

TEST(deque, move_construct) {
  std::deque<int> a = {1, 2, 3};
  std::deque<int> b(std::move(a));
  ASSERT_EQ(b.size(), 3U);
  ASSERT_EQ(a.size(), 0U);  // NOLINT(bugprone-use-after-move)
}

// ===========================================================================
// push_back / push_front
// ===========================================================================

TEST(deque, push_back) {
  std::deque<int> d;
  d.push_back(1);
  d.push_back(2);
  d.push_back(3);
  ASSERT_EQ(d.size(), 3U);
  ASSERT_EQ(d.front(), 1);
  ASSERT_EQ(d.back(), 3);
}

TEST(deque, push_front) {
  std::deque<int> d;
  d.push_front(1);
  d.push_front(2);
  d.push_front(3);
  ASSERT_EQ(d.size(), 3U);
  ASSERT_EQ(d.front(), 3);
  ASSERT_EQ(d.back(), 1);
}

TEST(deque, interleaved_push) {
  std::deque<int> d;
  d.push_back(2);
  d.push_front(1);
  d.push_back(3);
  d.push_front(0);
  ASSERT_EQ(d.size(), 4U);
  ASSERT_EQ(d[0], 0);
  ASSERT_EQ(d[1], 1);
  ASSERT_EQ(d[2], 2);
  ASSERT_EQ(d[3], 3);
}

// ===========================================================================
// pop_back / pop_front
// ===========================================================================

TEST(deque, pop_back) {
  std::deque<int> d = {1, 2, 3};
  d.pop_back();
  ASSERT_EQ(d.size(), 2U);
  ASSERT_EQ(d.back(), 2);
}

TEST(deque, pop_front) {
  std::deque<int> d = {1, 2, 3};
  d.pop_front();
  ASSERT_EQ(d.size(), 2U);
  ASSERT_EQ(d.front(), 2);
}

// ===========================================================================
// Element access
// ===========================================================================

TEST(deque, random_access) {
  std::deque<int> d = {10, 20, 30, 40};
  ASSERT_EQ(d[0], 10);
  ASSERT_EQ(d[2], 30);
  d[1] = 99;
  ASSERT_EQ(d[1], 99);
}

TEST(deque, front_back) {
  std::deque<int> d = {5, 10, 15};
  ASSERT_EQ(d.front(), 5);
  ASSERT_EQ(d.back(), 15);
}

// ===========================================================================
// Iteration
// ===========================================================================

TEST(deque, forward_iteration) {
  std::deque<int> d = {1, 2, 3};
  int sum = 0;
  for (auto& v : d) {
    sum += v;
  }
  ASSERT_EQ(sum, 6);
}

TEST(deque, iterator_arithmetic) {
  std::deque<int> d = {10, 20, 30, 40};
  auto it = d.begin();
  it += 2;
  ASSERT_EQ(*it, 30);
  it -= 1;
  ASSERT_EQ(*it, 20);
  ASSERT_EQ(d.end() - d.begin(), 4);
}

// ===========================================================================
// clear
// ===========================================================================

TEST(deque, clear) {
  std::deque<int> d = {1, 2, 3};
  d.clear();
  ASSERT_TRUE(d.empty());
  ASSERT_EQ(d.size(), 0U);
}

// ===========================================================================
// Grow through multiple doublings
// ===========================================================================

TEST(deque, grow_many) {
  std::deque<int> d;
  for (int i = 0; i < 100; ++i) {
    d.push_back(i);
  }
  ASSERT_EQ(d.size(), 100U);
  for (int i = 0; i < 100; ++i) {
    ASSERT_EQ(d[static_cast<size_t>(i)], i);
  }
}

TEST(deque, grow_front_many) {
  std::deque<int> d;
  for (int i = 0; i < 100; ++i) {
    d.push_front(i);
  }
  ASSERT_EQ(d.size(), 100U);
  for (int i = 0; i < 100; ++i) {
    ASSERT_EQ(d[static_cast<size_t>(i)], 99 - i);
  }
}

// ===========================================================================
// Comparison
// ===========================================================================

TEST(deque, equality) {
  std::deque<int> a = {1, 2, 3};
  std::deque<int> b = {1, 2, 3};
  std::deque<int> c = {1, 2, 4};
  ASSERT_TRUE(a == b);
  ASSERT_TRUE(a != c);
}
// NOLINTEND(misc-const-correctness)

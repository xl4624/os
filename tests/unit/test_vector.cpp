#include <ranges.h>
#include <vector.h>

#include "../framework/test.h"

namespace {

// Tracks construction/destruction counts.
struct Tracker {
  static int constructs;
  static int destructs;
  int value = 0;
  Tracker() { ++constructs; }
  explicit Tracker(int v) : value(v) { ++constructs; }
  Tracker(const Tracker& o) : value(o.value) { ++constructs; }
  Tracker(Tracker&& o) noexcept : value(o.value) { ++constructs; }
  ~Tracker() { ++destructs; }
  Tracker& operator=(const Tracker& o) = default;
  bool operator==(const Tracker& o) const { return value == o.value; }
};
int Tracker::constructs = 0;
int Tracker::destructs = 0;

}  // namespace

// ===========================================================================
// Construction
// ===========================================================================

TEST(vector, default_construct_is_empty) {
  std::vector<int> v;
  ASSERT_TRUE(v.empty());
  ASSERT_EQ(v.size(), 0U);
  ASSERT_EQ(v.capacity(), 0U);
  ASSERT_NULL(v.data());
}

TEST(vector, size_construct_value_inits) {
  std::vector<int> v(5);
  ASSERT_EQ(v.size(), 5U);
  for (size_t i = 0; i < 5; ++i) {
    ASSERT_EQ(v[i], 0);
  }
}

TEST(vector, fill_construct) {
  std::vector<int> v(4, 7);
  ASSERT_EQ(v.size(), 4U);
  for (size_t i = 0; i < 4; ++i) {
    ASSERT_EQ(v[i], 7);
  }
}

TEST(vector, initializer_list_construct) {
  std::vector<int> v = {1, 2, 3, 4};
  ASSERT_EQ(v.size(), 4U);
  ASSERT_EQ(v[0], 1);
  ASSERT_EQ(v[3], 4);
}

TEST(vector, copy_construct) {
  std::vector<int> a = {10, 20, 30};
  std::vector<int> b(a);
  ASSERT_EQ(b.size(), 3U);
  ASSERT_EQ(b[0], 10);
  ASSERT_EQ(b[2], 30);
  // Ensure deep copy.
  b[0] = 99;
  ASSERT_EQ(a[0], 10);
}

TEST(vector, move_construct) {
  std::vector<int> a = {1, 2, 3};
  const int* orig_data = a.data();
  std::vector<int> b(std::move(a));
  ASSERT_EQ(b.size(), 3U);
  ASSERT_EQ(b.data(), orig_data);
  ASSERT_EQ(a.size(), 0U);  // NOLINT(bugprone-use-after-move)
  ASSERT_NULL(a.data());    // NOLINT(bugprone-use-after-move)
}

TEST(vector, size_zero_construct) {
  const std::vector<int> v(0);
  ASSERT_TRUE(v.empty());
}

// ===========================================================================
// Assignment
// ===========================================================================

TEST(vector, copy_assign) {
  std::vector<int> a = {1, 2, 3};
  std::vector<int> b = {4, 5};
  b = a;
  ASSERT_EQ(b.size(), 3U);
  ASSERT_EQ(b[0], 1);
  ASSERT_EQ(b[2], 3);
  b[0] = 99;
  ASSERT_EQ(a[0], 1);
}

TEST(vector, move_assign) {
  std::vector<int> a = {1, 2, 3};
  const int* orig = a.data();
  std::vector<int> b;
  b = std::move(a);
  ASSERT_EQ(b.size(), 3U);
  ASSERT_EQ(b.data(), orig);
  ASSERT_TRUE(a.empty());  // NOLINT(bugprone-use-after-move)
}

TEST(vector, self_copy_assign) {
  std::vector<int> v = {1, 2, 3};
  v = v;  // NOLINT
  ASSERT_EQ(v.size(), 3U);
  ASSERT_EQ(v[0], 1);
}

TEST(vector, initializer_list_assign) {
  std::vector<int> v = {1, 2};
  v = {10, 20, 30};
  ASSERT_EQ(v.size(), 3U);
  ASSERT_EQ(v[1], 20);
}

TEST(vector, assign_fill) {
  std::vector<int> v = {1, 2, 3};
  v.assign(4, 5);
  ASSERT_EQ(v.size(), 4U);
  for (size_t i = 0; i < 4; ++i) {
    ASSERT_EQ(v[i], 5);
  }
}

TEST(vector, assign_initializer_list) {
  std::vector<int> v;
  v.assign({7, 8, 9});
  ASSERT_EQ(v.size(), 3U);
  ASSERT_EQ(v[0], 7);
}

// ===========================================================================
// Element access
// ===========================================================================

TEST(vector, index_operator) {
  std::vector<int> v = {10, 20, 30};
  ASSERT_EQ(v[0], 10);
  ASSERT_EQ(v[1], 20);
  ASSERT_EQ(v[2], 30);
}

TEST(vector, at) {
  std::vector<int> v = {1, 2, 3};
  ASSERT_EQ(v.at(0), 1);
  ASSERT_EQ(v.at(2), 3);
}

TEST(vector, front_and_back) {
  std::vector<int> v = {5, 10, 15};
  ASSERT_EQ(v.front(), 5);
  ASSERT_EQ(v.back(), 15);
}

TEST(vector, data_pointer) {
  std::vector<int> v = {1, 2, 3};
  const int* p = v.data();
  ASSERT_NOT_NULL(p);
  ASSERT_EQ(p[0], 1);
  ASSERT_EQ(p[2], 3);
}

TEST(vector, const_element_access) {
  const std::vector<int> v = {4, 5, 6};
  ASSERT_EQ(v[0], 4);
  ASSERT_EQ(v.front(), 4);
  ASSERT_EQ(v.back(), 6);
  ASSERT_EQ(v.at(1), 5);
  ASSERT_NOT_NULL(v.data());
}

// ===========================================================================
// Iterators
// ===========================================================================

TEST(vector, begin_end_loop) {
  const std::vector<int> v = {1, 2, 3, 4};
  int sum = 0;
  for (const int x : v) {
    sum += x;
  }
  ASSERT_EQ(sum, 10);
}

TEST(vector, cbegin_cend) {
  const std::vector<int> v = {2, 4, 6};
  int sum = 0;
  for (const int it : v) {
    sum += it;
  }
  ASSERT_EQ(sum, 12);
}

TEST(vector, rbegin_rend) {
  std::vector<int> v = {1, 2, 3};
  std::vector<int> rev;
  for (int& it : std::views::reverse(v)) {
    rev.push_back(it);
  }
  ASSERT_EQ(rev[0], 3);
  ASSERT_EQ(rev[1], 2);
  ASSERT_EQ(rev[2], 1);
}

// ===========================================================================
// capacity()
// ===========================================================================

TEST(vector, reserve_increases_capacity) {
  std::vector<int> v;
  v.reserve(16);
  ASSERT_TRUE(v.empty());
  ASSERT_EQ(v.capacity(), 16U);
}

TEST(vector, reserve_does_not_shrink) {
  std::vector<int> v;
  v.reserve(16);
  v.reserve(4);
  ASSERT_EQ(v.capacity(), 16U);
}

TEST(vector, shrink_to_fit_after_reserve) {
  std::vector<int> v = {1, 2, 3};
  v.reserve(100);
  ASSERT_EQ(v.capacity(), 100U);
  v.shrink_to_fit();
  ASSERT_EQ(v.capacity(), 3U);
  ASSERT_EQ(v[0], 1);
}

TEST(vector, shrink_to_fit_empty) {
  std::vector<int> v;
  v.reserve(10);
  v.shrink_to_fit();
  ASSERT_EQ(v.capacity(), 0U);
  ASSERT_NULL(v.data());
}

// ===========================================================================
// push_back() / pop_back()
// ===========================================================================

TEST(vector, push_back_grows) {
  std::vector<int> v;
  v.reserve(10);
  for (int i = 0; i < 10; ++i) {
    v.push_back(i);
  }
  ASSERT_EQ(v.size(), 10U);
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(v[static_cast<size_t>(i)], i);
  }
}

TEST(vector, push_back_move) {
  std::vector<std::vector<int>> v;
  std::vector<int> inner = {1, 2, 3};
  v.push_back(std::move(inner));
  ASSERT_EQ(v.size(), 1U);
  ASSERT_EQ(v[0].size(), 3U);
  ASSERT_TRUE(inner.empty());  // NOLINT(bugprone-use-after-move)
}

TEST(vector, pop_back) {
  std::vector<int> v = {1, 2, 3};
  v.pop_back();
  ASSERT_EQ(v.size(), 2U);
  ASSERT_EQ(v.back(), 2);
}

TEST(vector, push_and_pop_back) {
  std::vector<int> v;
  v.push_back(1);
  v.push_back(2);
  v.pop_back();
  ASSERT_EQ(v.size(), 1U);
  ASSERT_EQ(v.front(), 1);
}

// ===========================================================================
// emplace_back()
// ===========================================================================

TEST(vector, emplace_back_constructs_in_place) {
  struct Point {
    int x, y;
    Point(int a, int b) : x(a), y(b) {}
  };
  std::vector<Point> v;
  v.emplace_back(1, 2);
  v.emplace_back(3, 4);
  ASSERT_EQ(v.size(), 2U);
  ASSERT_EQ(v[0].x, 1);
  ASSERT_EQ(v[1].y, 4);
}

// ===========================================================================
// clear()
// ===========================================================================

TEST(vector, clear_empty_vector) {
  std::vector<int> v;
  v.clear();
  ASSERT_TRUE(v.empty());
}

TEST(vector, clear_nonempty_vector) {
  std::vector<int> v = {1, 2, 3};
  v.clear();
  ASSERT_TRUE(v.empty());
  ASSERT_EQ(v.size(), 0U);
  // Capacity is preserved.
  ASSERT_TRUE(v.capacity() > 0);
}

TEST(vector, clear_calls_destructors) {
  Tracker::constructs = 0;
  Tracker::destructs = 0;
  {
    std::vector<Tracker> v;
    v.reserve(2);  // avoid reallocation destructs during emplace
    v.emplace_back(1);
    v.emplace_back(2);
    v.clear();
    ASSERT_EQ(Tracker::destructs, 2);
    ASSERT_TRUE(v.empty());
  }
  // No additional destructs from v itself (capacity is still allocated but empty).
  ASSERT_EQ(Tracker::destructs, 2);
}

// ===========================================================================
// resize()
// ===========================================================================

TEST(vector, resize_grow) {
  std::vector<int> v = {1, 2};
  v.resize(5);
  ASSERT_EQ(v.size(), 5U);
  ASSERT_EQ(v[0], 1);
  ASSERT_EQ(v[2], 0);
}

TEST(vector, resize_shrink) {
  std::vector<int> v = {1, 2, 3, 4, 5};
  v.resize(3);
  ASSERT_EQ(v.size(), 3U);
  ASSERT_EQ(v.back(), 3);
}

TEST(vector, resize_with_value) {
  std::vector<int> v = {1, 2};
  v.resize(5, 99);
  ASSERT_EQ(v.size(), 5U);
  ASSERT_EQ(v[2], 99);
  ASSERT_EQ(v[4], 99);
}

TEST(vector, resize_shrink_calls_destructors) {
  Tracker::constructs = 0;
  Tracker::destructs = 0;
  std::vector<Tracker> v(4);
  v.resize(2);
  ASSERT_EQ(v.size(), 2U);
  ASSERT_EQ(Tracker::destructs, 2);
}

// ===========================================================================
// insert()
// ===========================================================================

TEST(vector, insert_at_end) {
  std::vector<int> v = {1, 2};
  auto* it = v.insert(v.end(), 3);
  ASSERT_EQ(v.size(), 3U);
  ASSERT_EQ(*it, 3);
  ASSERT_EQ(v[2], 3);
}

TEST(vector, insert_at_begin) {
  std::vector<int> v = {2, 3};
  auto* it = v.insert(v.begin(), 1);
  ASSERT_EQ(v.size(), 3U);
  ASSERT_EQ(*it, 1);
  ASSERT_EQ(v[0], 1);
  ASSERT_EQ(v[1], 2);
}

TEST(vector, insert_at_middle) {
  std::vector<int> v = {1, 3, 4};
  v.insert(v.begin() + 1, 2);
  ASSERT_EQ(v.size(), 4U);
  ASSERT_EQ(v[0], 1);
  ASSERT_EQ(v[1], 2);
  ASSERT_EQ(v[2], 3);
  ASSERT_EQ(v[3], 4);
}

TEST(vector, insert_move) {
  std::vector<std::vector<int>> v;
  v.push_back({1, 2});
  std::vector<int> inner = {3, 4};
  v.insert(v.end(), std::move(inner));
  ASSERT_EQ(v.size(), 2U);
  ASSERT_EQ(v[1].size(), 2U);
  ASSERT_TRUE(inner.empty());  // NOLINT(bugprone-use-after-move)
}

// ===========================================================================
// erase()
// ===========================================================================

TEST(vector, erase_first_element) {
  std::vector<int> v = {1, 2, 3};
  auto* it = v.erase(v.begin());
  ASSERT_EQ(v.size(), 2U);
  ASSERT_EQ(v[0], 2);
  ASSERT_EQ(*it, 2);
}

TEST(vector, erase_last_element) {
  std::vector<int> v = {1, 2, 3};
  auto* it = v.erase(v.end() - 1);
  ASSERT_EQ(v.size(), 2U);
  ASSERT_EQ(v.back(), 2);
  ASSERT_EQ(it, v.end());
}

TEST(vector, erase_middle_element) {
  std::vector<int> v = {1, 2, 3, 4};
  v.erase(v.begin() + 1);
  ASSERT_EQ(v.size(), 3U);
  ASSERT_EQ(v[0], 1);
  ASSERT_EQ(v[1], 3);
  ASSERT_EQ(v[2], 4);
}

TEST(vector, erase_range) {
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto* it = v.erase(v.begin() + 1, v.begin() + 4);
  ASSERT_EQ(v.size(), 2U);
  ASSERT_EQ(v[0], 1);
  ASSERT_EQ(v[1], 5);
  ASSERT_EQ(*it, 5);
}

TEST(vector, erase_all) {
  std::vector<int> v = {1, 2, 3};
  v.erase(v.begin(), v.end());
  ASSERT_TRUE(v.empty());
}

// ===========================================================================
// swap()
// ===========================================================================

TEST(vector, swap) {
  std::vector<int> a = {1, 2, 3};
  std::vector<int> b = {4, 5};
  a.swap(b);
  ASSERT_EQ(a.size(), 2U);
  ASSERT_EQ(b.size(), 3U);
  ASSERT_EQ(a[0], 4);
  ASSERT_EQ(b[0], 1);
}

// ===========================================================================
// Comparison operators
// ===========================================================================

TEST(vector, equality) {
  const std::vector<int> a = {1, 2, 3};
  const std::vector<int> b = {1, 2, 3};
  const std::vector<int> c = {1, 2, 4};
  ASSERT_TRUE(a == b);
  ASSERT_FALSE(a == c);
  ASSERT_TRUE(a != c);
  ASSERT_FALSE(a != b);
}

TEST(vector, empty_vectors_equal) {
  const std::vector<int> a;
  const std::vector<int> b;
  ASSERT_TRUE(a == b);
  ASSERT_FALSE(a != b);
}

TEST(vector, different_sizes_not_equal) {
  const std::vector<int> a = {1, 2};
  const std::vector<int> b = {1, 2, 3};
  ASSERT_FALSE(a == b);
  ASSERT_TRUE(a != b);
}

TEST(vector, ordering) {
  const std::vector<int> a = {1, 2, 3};
  const std::vector<int> b = {1, 2, 4};
  ASSERT_TRUE(a < b);
  ASSERT_FALSE(b < a);
  ASSERT_TRUE(b > a);
  ASSERT_TRUE(a <= b);
  ASSERT_TRUE(a <= a);
  ASSERT_TRUE(b >= a);
  ASSERT_TRUE(b >= b);
}

// ===========================================================================
// Destructor
// ===========================================================================

TEST(vector, destructor_calls_element_destructors) {
  Tracker::constructs = 0;
  Tracker::destructs = 0;
  {
    std::vector<Tracker> v;
    v.reserve(3);  // avoid reallocation destructs during emplace
    v.emplace_back(1);
    v.emplace_back(2);
    v.emplace_back(3);
    ASSERT_EQ(Tracker::destructs, 0);
  }
  ASSERT_EQ(Tracker::destructs, 3);
}

TEST(vector, copy_assign_destroys_old_elements) {
  Tracker::constructs = 0;
  Tracker::destructs = 0;
  const std::vector<Tracker> a(3);
  std::vector<Tracker> b(2);
  b = a;
  // b's original 2 elements are destroyed.
  ASSERT_EQ(Tracker::destructs, 2);
}

// ===========================================================================
// Growth and reallocation
// ===========================================================================

TEST(vector, growth_factor) {
  std::vector<int> v;
  v.push_back(1);
  ASSERT_EQ(v.capacity(), 1U);
  v.push_back(2);
  ASSERT_EQ(v.capacity(), 2U);
  v.push_back(3);
  ASSERT_EQ(v.capacity(), 4U);
  v.push_back(4);
  ASSERT_EQ(v.capacity(), 4U);
  v.push_back(5);
  ASSERT_EQ(v.capacity(), 8U);
}

TEST(vector, elements_survive_reallocation) {
  std::vector<int> v;
  v.reserve(100);
  for (int i = 0; i < 100; ++i) {
    v.push_back(i);
  }
  for (int i = 0; i < 100; ++i) {
    ASSERT_EQ(v[static_cast<size_t>(i)], i);
  }
}

// ===========================================================================
// Non-trivial element type
// ===========================================================================

TEST(vector, vector_of_vectors) {
  std::vector<std::vector<int>> v;
  v.push_back({1, 2, 3});
  v.push_back({4, 5});
  ASSERT_EQ(v.size(), 2U);
  ASSERT_EQ(v[0].size(), 3U);
  ASSERT_EQ(v[1][0], 4);
}

TEST(vector, resize_non_trivial_calls_constructors) {
  Tracker::constructs = 0;
  Tracker::destructs = 0;
  std::vector<Tracker> v;
  v.resize(3);
  ASSERT_EQ(v.size(), 3U);
  ASSERT_EQ(Tracker::constructs, 3);
  v.resize(1);
  ASSERT_EQ(Tracker::destructs, 2);
}

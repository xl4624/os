// NOLINTBEGIN(misc-const-correctness)
#include <list.h>

#include "../framework/test.h"

namespace {

struct Tracker {
  static int alive;
  int value = 0;
  Tracker() { ++alive; }
  explicit Tracker(int v) : value(v) { ++alive; }
  Tracker(const Tracker& o) : value(o.value) { ++alive; }
  Tracker(Tracker&& o) noexcept : value(o.value) { ++alive; }
  ~Tracker() { --alive; }
  Tracker& operator=(const Tracker&) = default;
  bool operator==(const Tracker& o) const { return value == o.value; }
};

int Tracker::alive = 0;

}  // namespace

// ===========================================================================
// Construction
// ===========================================================================

TEST(list, default_construct_is_empty) {
  std::list<int> l;
  ASSERT_TRUE(l.empty());
  ASSERT_EQ(l.size(), 0U);
}

TEST(list, fill_construct) {
  std::list<int> l(3, 42);
  ASSERT_EQ(l.size(), 3U);
  for (auto& v : l) {
    ASSERT_EQ(v, 42);
  }
}

TEST(list, initializer_list_construct) {
  std::list<int> l = {1, 2, 3, 4};
  ASSERT_EQ(l.size(), 4U);
  ASSERT_EQ(l.front(), 1);
  ASSERT_EQ(l.back(), 4);
}

TEST(list, copy_construct) {
  std::list<int> a = {10, 20, 30};
  std::list<int> b(a);
  ASSERT_EQ(b.size(), 3U);
  ASSERT_EQ(b.front(), 10);
  ASSERT_EQ(b.back(), 30);
  // Ensure deep copy.
  b.front() = 99;
  ASSERT_EQ(a.front(), 10);
}

TEST(list, move_construct) {
  std::list<int> a = {1, 2, 3};
  std::list<int> b(std::move(a));
  ASSERT_EQ(b.size(), 3U);
  ASSERT_EQ(a.size(), 0U);  // NOLINT(bugprone-use-after-move)
  ASSERT_TRUE(a.empty());   // NOLINT(bugprone-use-after-move)
}

// ===========================================================================
// push_back / push_front
// ===========================================================================

TEST(list, push_back) {
  std::list<int> l;
  l.push_back(1);
  l.push_back(2);
  l.push_back(3);
  ASSERT_EQ(l.size(), 3U);
  ASSERT_EQ(l.front(), 1);
  ASSERT_EQ(l.back(), 3);
}

TEST(list, push_front) {
  std::list<int> l;
  l.push_front(1);
  l.push_front(2);
  l.push_front(3);
  ASSERT_EQ(l.size(), 3U);
  ASSERT_EQ(l.front(), 3);
  ASSERT_EQ(l.back(), 1);
}

// ===========================================================================
// pop_back / pop_front
// ===========================================================================

TEST(list, pop_back) {
  std::list<int> l = {1, 2, 3};
  l.pop_back();
  ASSERT_EQ(l.size(), 2U);
  ASSERT_EQ(l.back(), 2);
}

TEST(list, pop_front) {
  std::list<int> l = {1, 2, 3};
  l.pop_front();
  ASSERT_EQ(l.size(), 2U);
  ASSERT_EQ(l.front(), 2);
}

// ===========================================================================
// Iteration
// ===========================================================================

TEST(list, forward_iteration) {
  std::list<int> l = {10, 20, 30};
  int sum = 0;
  for (auto& v : l) {
    sum += v;
  }
  ASSERT_EQ(sum, 60);
}

TEST(list, backward_iteration) {
  std::list<int> l = {1, 2, 3};
  auto it = l.end();
  --it;
  ASSERT_EQ(*it, 3);
  --it;
  ASSERT_EQ(*it, 2);
  --it;
  ASSERT_EQ(*it, 1);
}

// ===========================================================================
// insert / erase
// ===========================================================================

TEST(list, insert_at_begin) {
  std::list<int> l = {2, 3};
  l.insert(l.begin(), 1);
  ASSERT_EQ(l.size(), 3U);
  ASSERT_EQ(l.front(), 1);
}

TEST(list, insert_at_end) {
  std::list<int> l = {1, 2};
  l.insert(l.end(), 3);
  ASSERT_EQ(l.back(), 3);
}

TEST(list, erase_middle) {
  std::list<int> l = {1, 2, 3};
  auto it = l.begin();
  ++it;  // points to 2
  it = l.erase(it);
  ASSERT_EQ(*it, 3);
  ASSERT_EQ(l.size(), 2U);
  ASSERT_EQ(l.front(), 1);
  ASSERT_EQ(l.back(), 3);
}

// ===========================================================================
// emplace
// ===========================================================================

TEST(list, emplace_back) {
  std::list<Tracker> l;
  Tracker::alive = 0;
  l.emplace_back(42);
  ASSERT_EQ(l.size(), 1U);
  ASSERT_EQ(l.front().value, 42);
  ASSERT_EQ(Tracker::alive, 1);
}

// ===========================================================================
// clear
// ===========================================================================

TEST(list, clear) {
  Tracker::alive = 0;
  {
    std::list<Tracker> l;
    l.emplace_back(1);
    l.emplace_back(2);
    l.emplace_back(3);
    ASSERT_EQ(Tracker::alive, 3);
    l.clear();
    ASSERT_EQ(Tracker::alive, 0);
    ASSERT_TRUE(l.empty());
  }
  ASSERT_EQ(Tracker::alive, 0);
}

// ===========================================================================
// splice
// ===========================================================================

TEST(list, splice_all) {
  std::list<int> a = {1, 2};
  std::list<int> b = {3, 4};
  a.splice(a.end(), b);
  ASSERT_EQ(a.size(), 4U);
  ASSERT_TRUE(b.empty());
  auto it = a.begin();
  ASSERT_EQ(*it, 1);
  ++it;
  ASSERT_EQ(*it, 2);
  ++it;
  ASSERT_EQ(*it, 3);
  ++it;
  ASSERT_EQ(*it, 4);
}

// ===========================================================================
// Comparison
// ===========================================================================

TEST(list, equality) {
  std::list<int> a = {1, 2, 3};
  std::list<int> b = {1, 2, 3};
  std::list<int> c = {1, 2, 4};
  ASSERT_TRUE(a == b);
  ASSERT_TRUE(a != c);
}

// ===========================================================================
// Destructor
// ===========================================================================

TEST(list, destructor_frees_all) {
  Tracker::alive = 0;
  {
    std::list<Tracker> l;
    l.emplace_back(1);
    l.emplace_back(2);
    l.emplace_back(3);
  }
  ASSERT_EQ(Tracker::alive, 0);
}
// NOLINTEND(misc-const-correctness)

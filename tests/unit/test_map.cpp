// NOLINTBEGIN(misc-const-correctness)
#include <map.h>

#include "../framework/test.h"

// ===========================================================================
// Construction
// ===========================================================================

TEST(map, default_construct_is_empty) {
  std::map<int, int> m;
  ASSERT_TRUE(m.empty());
  ASSERT_EQ(m.size(), 0U);
}

TEST(map, initializer_list_construct) {
  std::map<int, int> m = {{3, 30}, {1, 10}, {2, 20}};
  ASSERT_EQ(m.size(), 3U);
  ASSERT_EQ(m[1], 10);
  ASSERT_EQ(m[2], 20);
  ASSERT_EQ(m[3], 30);
}

// ===========================================================================
// Insert and lookup
// ===========================================================================

TEST(map, insert_and_find) {
  std::map<int, int> m;
  auto [it, inserted] = m.insert({42, 100});
  ASSERT_TRUE(inserted);
  ASSERT_EQ(it->first, 42);
  ASSERT_EQ(it->second, 100);

  auto [it2, inserted2] = m.insert({42, 999});
  ASSERT_FALSE(inserted2);
  ASSERT_EQ(it2->second, 100);  // Original value kept.
}

TEST(map, operator_bracket) {
  std::map<int, int> m;
  m[1] = 10;
  m[2] = 20;
  ASSERT_EQ(m[1], 10);
  ASSERT_EQ(m[2], 20);
  ASSERT_EQ(m.size(), 2U);
}

TEST(map, operator_bracket_default_inserts) {
  std::map<int, int> m;
  int& v = m[99];
  ASSERT_EQ(v, 0);
  ASSERT_EQ(m.size(), 1U);
}

TEST(map, find_missing) {
  std::map<int, int> m = {{1, 10}};
  auto it = m.find(999);
  ASSERT_TRUE(it == m.end());
}

TEST(map, contains) {
  std::map<int, int> m = {{1, 10}, {2, 20}};
  ASSERT_TRUE(m.contains(1));
  ASSERT_TRUE(m.contains(2));
  ASSERT_FALSE(m.contains(3));
}

// ===========================================================================
// Ordered iteration
// ===========================================================================

TEST(map, iterates_in_order) {
  std::map<int, int> m;
  m[5] = 50;
  m[1] = 10;
  m[3] = 30;
  m[2] = 20;
  m[4] = 40;

  int expected_key = 1;
  for (const auto& [k, v] : m) {
    ASSERT_EQ(k, expected_key);
    ASSERT_EQ(v, expected_key * 10);
    ++expected_key;
  }
  ASSERT_EQ(expected_key, 6);
}

TEST(map, reverse_iteration) {
  std::map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
  auto it = m.end();
  --it;
  ASSERT_EQ(it->first, 3);
  --it;
  ASSERT_EQ(it->first, 2);
  --it;
  ASSERT_EQ(it->first, 1);
}

// ===========================================================================
// Erase
// ===========================================================================

TEST(map, erase_by_key) {
  std::map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
  size_t erased = m.erase(2);
  ASSERT_EQ(erased, 1U);
  ASSERT_EQ(m.size(), 2U);
  ASSERT_FALSE(m.contains(2));
  // Remaining elements still in order.
  auto it = m.begin();
  ASSERT_EQ(it->first, 1);
  ++it;
  ASSERT_EQ(it->first, 3);
}

TEST(map, erase_missing_key) {
  std::map<int, int> m = {{1, 10}};
  size_t erased = m.erase(999);
  ASSERT_EQ(erased, 0U);
  ASSERT_EQ(m.size(), 1U);
}

TEST(map, erase_by_iterator) {
  std::map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
  auto it = m.find(2);
  auto next = m.erase(it);
  ASSERT_EQ(m.size(), 2U);
  ASSERT_FALSE(m.contains(2));
  ASSERT_EQ(next->first, 3);
}

TEST(map, erase_root) {
  // Insert in order that makes the root likely to be erased.
  std::map<int, int> m;
  m[2] = 20;
  m[1] = 10;
  m[3] = 30;
  m.erase(2);
  ASSERT_EQ(m.size(), 2U);
  ASSERT_TRUE(m.contains(1));
  ASSERT_TRUE(m.contains(3));
}

// ===========================================================================
// Clear
// ===========================================================================

TEST(map, clear) {
  std::map<int, int> m = {{1, 10}, {2, 20}};
  m.clear();
  ASSERT_TRUE(m.empty());
  ASSERT_EQ(m.size(), 0U);
}

// ===========================================================================
// Copy and move
// ===========================================================================

TEST(map, copy_construct) {
  std::map<int, int> a = {{1, 10}, {2, 20}};
  std::map<int, int> b(a);
  ASSERT_EQ(b.size(), 2U);
  ASSERT_EQ(b[1], 10);
  ASSERT_EQ(b[2], 20);
  b[1] = 99;
  ASSERT_EQ(a[1], 10);
}

TEST(map, move_construct) {
  std::map<int, int> a = {{1, 10}};
  std::map<int, int> b(std::move(a));
  ASSERT_EQ(b.size(), 1U);
  ASSERT_EQ(b[1], 10);
  ASSERT_EQ(a.size(), 0U);  // NOLINT(bugprone-use-after-move)
}

// ===========================================================================
// Many insertions (stress RB tree balancing)
// ===========================================================================

TEST(map, many_inserts_and_ordered) {
  std::map<int, int> m;
  // Insert in reverse to stress left-rotation paths.
  for (int i = 99; i >= 0; --i) {
    m[i] = i * 10;
  }
  ASSERT_EQ(m.size(), 100U);
  int expected = 0;
  for (const auto& [k, v] : m) {
    ASSERT_EQ(k, expected);
    ASSERT_EQ(v, expected * 10);
    ++expected;
  }
  ASSERT_EQ(expected, 100);
}

TEST(map, many_erases) {
  std::map<int, int> m;
  for (int i = 0; i < 50; ++i) {
    m[i] = i;
  }
  // Erase even keys.
  for (int i = 0; i < 50; i += 2) {
    m.erase(i);
  }
  ASSERT_EQ(m.size(), 25U);
  // Remaining keys are odd, still in order.
  int expected = 1;
  for (const auto& [k, v] : m) {
    ASSERT_EQ(k, expected);
    expected += 2;
  }
}
// NOLINTEND(misc-const-correctness)

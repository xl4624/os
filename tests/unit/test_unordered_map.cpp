#include <unordered_map.h>

#include "../framework/test.h"

// ===========================================================================
// Construction
// ===========================================================================

TEST(unordered_map, default_construct_is_empty) {
  std::unordered_map<int, int> m;
  ASSERT_TRUE(m.empty());
  ASSERT_EQ(m.size(), 0U);
}

TEST(unordered_map, initializer_list_construct) {
  std::unordered_map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
  ASSERT_EQ(m.size(), 3U);
  ASSERT_EQ(m[1], 10);
  ASSERT_EQ(m[2], 20);
  ASSERT_EQ(m[3], 30);
}

// ===========================================================================
// Insert and lookup
// ===========================================================================

TEST(unordered_map, insert_and_find) {
  std::unordered_map<int, int> m;
  auto [it, inserted] = m.insert({42, 100});
  ASSERT_TRUE(inserted);
  ASSERT_EQ(it->first, 42);
  ASSERT_EQ(it->second, 100);

  auto [it2, inserted2] = m.insert({42, 999});
  ASSERT_FALSE(inserted2);
  ASSERT_EQ(it2->second, 100);  // Original value kept.
}

TEST(unordered_map, operator_bracket) {
  std::unordered_map<int, int> m;
  m[1] = 10;
  m[2] = 20;
  ASSERT_EQ(m[1], 10);
  ASSERT_EQ(m[2], 20);
  ASSERT_EQ(m.size(), 2U);
}

TEST(unordered_map, operator_bracket_default_inserts) {
  std::unordered_map<int, int> m;
  int& v = m[99];
  ASSERT_EQ(v, 0);  // Default-constructed.
  ASSERT_EQ(m.size(), 1U);
}

TEST(unordered_map, find_missing) {
  std::unordered_map<int, int> m = {{1, 10}};
  auto it = m.find(999);
  ASSERT_TRUE(it == m.end());
}

TEST(unordered_map, contains) {
  std::unordered_map<int, int> m = {{1, 10}, {2, 20}};
  ASSERT_TRUE(m.contains(1));
  ASSERT_TRUE(m.contains(2));
  ASSERT_FALSE(m.contains(3));
}

TEST(unordered_map, count) {
  std::unordered_map<int, int> m = {{5, 50}};
  ASSERT_EQ(m.count(5), 1U);
  ASSERT_EQ(m.count(6), 0U);
}

// ===========================================================================
// Erase
// ===========================================================================

TEST(unordered_map, erase_by_key) {
  std::unordered_map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
  size_t erased = m.erase(2);
  ASSERT_EQ(erased, 1U);
  ASSERT_EQ(m.size(), 2U);
  ASSERT_FALSE(m.contains(2));
}

TEST(unordered_map, erase_missing_key) {
  std::unordered_map<int, int> m = {{1, 10}};
  size_t erased = m.erase(999);
  ASSERT_EQ(erased, 0U);
  ASSERT_EQ(m.size(), 1U);
}

TEST(unordered_map, erase_by_iterator) {
  std::unordered_map<int, int> m = {{1, 10}, {2, 20}};
  auto it = m.find(1);
  m.erase(it);
  ASSERT_EQ(m.size(), 1U);
  ASSERT_FALSE(m.contains(1));
  ASSERT_TRUE(m.contains(2));
}

// ===========================================================================
// Clear
// ===========================================================================

TEST(unordered_map, clear) {
  std::unordered_map<int, int> m = {{1, 10}, {2, 20}};
  m.clear();
  ASSERT_TRUE(m.empty());
  ASSERT_EQ(m.size(), 0U);
}

// ===========================================================================
// Iteration
// ===========================================================================

TEST(unordered_map, iterate_all) {
  std::unordered_map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
  int key_sum = 0;
  int val_sum = 0;
  for (const auto& [k, v] : m) {
    key_sum += k;
    val_sum += v;
  }
  ASSERT_EQ(key_sum, 6);
  ASSERT_EQ(val_sum, 60);
}

// ===========================================================================
// Copy and move
// ===========================================================================

TEST(unordered_map, copy_construct) {
  std::unordered_map<int, int> a = {{1, 10}, {2, 20}};
  std::unordered_map<int, int> b(a);
  ASSERT_EQ(b.size(), 2U);
  ASSERT_EQ(b[1], 10);
  ASSERT_EQ(b[2], 20);
  // Ensure deep copy.
  b[1] = 99;
  ASSERT_EQ(a[1], 10);
}

TEST(unordered_map, move_construct) {
  std::unordered_map<int, int> a = {{1, 10}};
  std::unordered_map<int, int> b(std::move(a));
  ASSERT_EQ(b.size(), 1U);
  ASSERT_EQ(b[1], 10);
  ASSERT_EQ(a.size(), 0U);  // NOLINT(bugprone-use-after-move)
}

// ===========================================================================
// Rehash (insert enough to trigger growth)
// ===========================================================================

TEST(unordered_map, rehash_on_many_inserts) {
  std::unordered_map<int, int> m;
  for (int i = 0; i < 100; ++i) {
    m[i] = i * 10;
  }
  ASSERT_EQ(m.size(), 100U);
  for (int i = 0; i < 100; ++i) {
    ASSERT_TRUE(m.contains(i));
    ASSERT_EQ(m[i], i * 10);
  }
}

// ===========================================================================
// Emplace
// ===========================================================================

TEST(unordered_map, emplace) {
  std::unordered_map<int, int> m;
  auto [it, inserted] = m.emplace(5, 50);
  ASSERT_TRUE(inserted);
  ASSERT_EQ(it->first, 5);
  ASSERT_EQ(it->second, 50);

  auto [it2, inserted2] = m.emplace(5, 999);
  ASSERT_FALSE(inserted2);
  ASSERT_EQ(it2->second, 50);
}

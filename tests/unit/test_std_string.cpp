#include <string.h>

#include "../framework/test.h"

// ============================================================================
// Construction
// ============================================================================

TEST(std_string, default_ctor_empty) {
  const std::string s;
  ASSERT_EQ(s.size(), 0U);
  ASSERT(s.empty());
  ASSERT_STR_EQ(s.c_str(), "");
}

TEST(std_string, ctor_from_cstr) {
  const std::string s("hello");
  ASSERT_EQ(s.size(), 5U);
  ASSERT_STR_EQ(s.c_str(), "hello");
}

TEST(std_string, ctor_from_cstr_and_len) {
  const std::string s("hello world", 5);
  ASSERT_EQ(s.size(), 5U);
  ASSERT_STR_EQ(s.c_str(), "hello");
}

TEST(std_string, ctor_fill) {
  const std::string s(4, 'x');
  ASSERT_EQ(s.size(), 4U);
  ASSERT_STR_EQ(s.c_str(), "xxxx");
}

TEST(std_string, copy_ctor) {
  std::string a("copy");
  const std::string b(a);
  ASSERT_STR_EQ(b.c_str(), "copy");
  // Mutating a must not affect b.
  a += "!";
  ASSERT_STR_EQ(b.c_str(), "copy");
}

TEST(std_string, move_ctor) {
  std::string a("moved");
  const std::string b(static_cast<std::string&&>(a));
  ASSERT_STR_EQ(b.c_str(), "moved");
  ASSERT(a.empty());
}

// ============================================================================
// Assignment
// ============================================================================

TEST(std_string, copy_assign) {
  const std::string a("hello");
  std::string b;
  b = a;
  ASSERT_STR_EQ(b.c_str(), "hello");
}

TEST(std_string, move_assign) {
  std::string a("move");
  std::string b;
  b = static_cast<std::string&&>(a);
  ASSERT_STR_EQ(b.c_str(), "move");
  ASSERT(a.empty());
}

TEST(std_string, assign_cstr) {
  std::string s;
  s = "assigned";
  ASSERT_STR_EQ(s.c_str(), "assigned");
}

TEST(std_string, self_assign) {
  std::string s("self");
  s = s;
  ASSERT_STR_EQ(s.c_str(), "self");
}

// ============================================================================
// Append / operator+= / operator+
// ============================================================================

TEST(std_string, append_cstr) {
  std::string s("hello");
  s.append(" world");
  ASSERT_STR_EQ(s.c_str(), "hello world");
}

TEST(std_string, append_string) {
  std::string a("foo");
  const std::string b("bar");
  a.append(b);
  ASSERT_STR_EQ(a.c_str(), "foobar");
}

TEST(std_string, append_fill) {
  std::string s("ab");
  s.append(3, 'c');
  ASSERT_STR_EQ(s.c_str(), "abccc");
}

TEST(std_string, operator_plus_eq_string) {
  std::string s("hello");
  s += std::string(" world");
  ASSERT_STR_EQ(s.c_str(), "hello world");
}

TEST(std_string, operator_plus_eq_cstr) {
  std::string s("hello");
  s += " world";
  ASSERT_STR_EQ(s.c_str(), "hello world");
}

TEST(std_string, operator_plus_eq_char) {
  std::string s("hello");
  s += '!';
  ASSERT_STR_EQ(s.c_str(), "hello!");
}

TEST(std_string, operator_plus_strings) {
  const std::string a("foo");
  const std::string b("bar");
  const std::string c = a + b;
  ASSERT_STR_EQ(c.c_str(), "foobar");
}

TEST(std_string, operator_plus_cstr_rhs) {
  const std::string a("foo");
  const std::string c = a + "bar";
  ASSERT_STR_EQ(c.c_str(), "foobar");
}

TEST(std_string, operator_plus_cstr_lhs) {
  const std::string b("bar");
  const std::string c = "foo" + b;
  ASSERT_STR_EQ(c.c_str(), "foobar");
}

// ============================================================================
// push_back / pop_back
// ============================================================================

TEST(std_string, push_back) {
  std::string s;
  s.push_back('a');
  s.push_back('b');
  s.push_back('c');
  ASSERT_STR_EQ(s.c_str(), "abc");
  ASSERT_EQ(s.size(), 3U);
}

TEST(std_string, pop_back) {
  std::string s("hello");
  s.pop_back();
  ASSERT_STR_EQ(s.c_str(), "hell");
  ASSERT_EQ(s.size(), 4U);
}

// ============================================================================
// Element access
// ============================================================================

TEST(std_string, index_operator) {
  std::string s("hello");
  ASSERT_EQ(s[0], 'h');
  ASSERT_EQ(s[4], 'o');
  s[0] = 'H';
  ASSERT_STR_EQ(s.c_str(), "Hello");
}

TEST(std_string, front_back) {
  std::string s("hello");
  ASSERT_EQ(s.front(), 'h');
  ASSERT_EQ(s.back(), 'o');
}

// ============================================================================
// Capacity
// ============================================================================

TEST(std_string, reserve_increases_capacity) {
  std::string s;
  s.reserve(100);
  ASSERT(s.capacity() >= 100U);
  ASSERT_EQ(s.size(), 0U);
}

TEST(std_string, resize_extend) {
  std::string s("hi");
  s.resize(5, 'x');
  ASSERT_EQ(s.size(), 5U);
  ASSERT_STR_EQ(s.c_str(), "hixxx");
}

TEST(std_string, resize_shrink) {
  std::string s("hello");
  s.resize(3);
  ASSERT_EQ(s.size(), 3U);
  ASSERT_STR_EQ(s.c_str(), "hel");
}

TEST(std_string, clear) {
  std::string s("hello");
  s.clear();
  ASSERT(s.empty());
  ASSERT_EQ(s.size(), 0U);
}

// ============================================================================
// Find
// ============================================================================

TEST(std_string, find_char_found) {
  const std::string s("hello world");
  ASSERT_EQ(s.find('o'), 4U);
}

TEST(std_string, find_char_not_found) {
  const std::string s("hello");
  ASSERT_EQ(s.find('z'), std::string::npos);
}

TEST(std_string, find_char_with_pos) {
  const std::string s("hello world");
  ASSERT_EQ(s.find('o', 5), 7U);
}

TEST(std_string, find_cstr_found) {
  const std::string s("hello world");
  ASSERT_EQ(s.find("world"), 6U);
}

TEST(std_string, find_cstr_not_found) {
  const std::string s("hello");
  ASSERT_EQ(s.find("xyz"), std::string::npos);
}

TEST(std_string, rfind_char) {
  const std::string s("hello world");
  ASSERT_EQ(s.rfind('o'), 7U);
}

TEST(std_string, rfind_not_found) {
  const std::string s("hello");
  ASSERT_EQ(s.rfind('z'), std::string::npos);
}

// ============================================================================
// Substr
// ============================================================================

TEST(std_string, substr_basic) {
  const std::string s("hello world");
  const std::string sub = s.substr(6, 5);
  ASSERT_STR_EQ(sub.c_str(), "world");
}

TEST(std_string, substr_to_end) {
  const std::string s("hello world");
  const std::string sub = s.substr(6);
  ASSERT_STR_EQ(sub.c_str(), "world");
}

TEST(std_string, substr_empty) {
  const std::string s("hello");
  const std::string sub = s.substr(2, 0);
  ASSERT(sub.empty());
}

// ============================================================================
// Insert / erase
// ============================================================================

TEST(std_string, insert_at_start) {
  std::string s("world");
  s.insert(0, "hello ");
  ASSERT_STR_EQ(s.c_str(), "hello world");
}

TEST(std_string, insert_in_middle) {
  std::string s("helo");
  s.insert(3, "l");
  ASSERT_STR_EQ(s.c_str(), "hello");
}

TEST(std_string, erase_middle) {
  std::string s("hello world");
  s.erase(5, 6);
  ASSERT_STR_EQ(s.c_str(), "hello");
}

TEST(std_string, erase_to_end) {
  std::string s("hello world");
  s.erase(5);
  ASSERT_STR_EQ(s.c_str(), "hello");
}

// ============================================================================
// Compare / relational operators
// ============================================================================

TEST(std_string, compare_equal) {
  const std::string a("hello");
  const std::string b("hello");
  ASSERT_EQ(a.compare(b), 0);
}

TEST(std_string, compare_less) {
  const std::string a("abc");
  const std::string b("def");
  ASSERT(a.compare(b) < 0);
}

TEST(std_string, compare_greater) {
  const std::string a("def");
  const std::string b("abc");
  ASSERT(a.compare(b) > 0);
}

TEST(std_string, operator_eq) {
  ASSERT(std::string("hello") == std::string("hello"));
  ASSERT(std::string("hello") == "hello");
  ASSERT("hello" == std::string("hello"));
}

TEST(std_string, operator_ne) {
  ASSERT(std::string("hello") != std::string("world"));
  ASSERT(std::string("hello") != "world");
}

TEST(std_string, operator_lt) {
  ASSERT(std::string("abc") < std::string("abd"));
  ASSERT(!(std::string("abd") < std::string("abc")));
}

// ============================================================================
// Iterators
// ============================================================================

TEST(std_string, range_for) {
  const std::string s("hello");
  int count = 0;
  for (const char c : s) {
    (void)c;
    ++count;
  }
  ASSERT_EQ(count, 5);
}

TEST(std_string, begin_end) {
  std::string s("abc");
  ASSERT_EQ(*s.begin(), 'a');
  ASSERT_EQ(*(s.end() - 1), 'c');
}

TEST(std_string, reverse_iterator) {
  std::string s("abc");
  std::string rev;
  for (auto it = s.rbegin(); it != s.rend(); ++it) {
    rev.push_back(*it);
  }
  ASSERT_STR_EQ(rev.c_str(), "cba");
}

// ============================================================================
// to_string
// ============================================================================

TEST(std_string, to_string_positive) { ASSERT_STR_EQ(std::to_string(42).c_str(), "42"); }

TEST(std_string, to_string_zero) { ASSERT_STR_EQ(std::to_string(0).c_str(), "0"); }

TEST(std_string, to_string_negative) { ASSERT_STR_EQ(std::to_string(-7).c_str(), "-7"); }

TEST(std_string, to_string_unsigned) { ASSERT_STR_EQ(std::to_string(123U).c_str(), "123"); }

// ============================================================================
// Growth / stress
// ============================================================================

TEST(std_string, growth_many_appends) {
  std::string s;
  for (int i = 0; i < 100; ++i) {
    s.push_back('a');
  }
  ASSERT_EQ(s.size(), 100U);
  for (size_t i = 0; i < s.size(); ++i) {
    ASSERT_EQ(s[i], 'a');
  }
}

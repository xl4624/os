#include <optional.h>

#include "../framework/test.h"

namespace {

// Tracks construction and destruction calls.
struct Tracked {
  static int instances;
  int value;
  Tracked(int v) : value(v) { ++instances; }
  Tracked(const Tracked& o) : value(o.value) { ++instances; }
  Tracked(Tracked&& o) noexcept : value(o.value) {
    o.value = -1;
    ++instances;
  }
  ~Tracked() { --instances; }
};

int Tracked::instances = 0;

}  // namespace

// ===========================================================================
// Construction
// ===========================================================================

TEST(optional, default_is_empty) {
  std::optional<int> o;
  ASSERT_FALSE(o.has_value());
  ASSERT_FALSE(static_cast<bool>(o));
}

TEST(optional, nullopt_is_empty) {
  std::optional<int> o = std::nullopt;
  ASSERT_FALSE(o.has_value());
}

TEST(optional, value_construction) {
  std::optional<int> o(42);
  ASSERT_TRUE(o.has_value());
  ASSERT_EQ(*o, 42);
}

TEST(optional, implicit_value_construction) {
  std::optional<int> o = 7;
  ASSERT_TRUE(o.has_value());
  ASSERT_EQ(*o, 7);
}

TEST(optional, copy_construction_engaged) {
  std::optional<int> a = 10;
  std::optional<int> b = a;
  ASSERT_TRUE(b.has_value());
  ASSERT_EQ(*b, 10);
}

TEST(optional, copy_construction_empty) {
  std::optional<int> a;
  std::optional<int> b = a;
  ASSERT_FALSE(b.has_value());
}

TEST(optional, move_construction_engaged) {
  std::optional<int> a = 99;
  std::optional<int> b = std::move(a);
  ASSERT_TRUE(b.has_value());
  ASSERT_EQ(*b, 99);
  ASSERT_FALSE(a.has_value());
}

TEST(optional, move_construction_empty) {
  std::optional<int> a;
  std::optional<int> b = std::move(a);
  ASSERT_FALSE(b.has_value());
}

// ===========================================================================
// Destructor
// ===========================================================================

TEST(optional, destructor_called_on_reset) {
  Tracked::instances = 0;
  {
    std::optional<Tracked> o = Tracked(5);
    ASSERT_EQ(Tracked::instances, 1);
    o.reset();
    ASSERT_EQ(Tracked::instances, 0);
  }
}

TEST(optional, destructor_called_on_scope_exit) {
  Tracked::instances = 0;
  {
    std::optional<Tracked> o = Tracked(3);
    ASSERT_EQ(Tracked::instances, 1);
  }
  ASSERT_EQ(Tracked::instances, 0);
}

// ===========================================================================
// Assignment
// ===========================================================================

TEST(optional, nullopt_assignment_clears) {
  std::optional<int> o = 5;
  o = std::nullopt;
  ASSERT_FALSE(o.has_value());
}

TEST(optional, copy_assignment_engaged) {
  std::optional<int> a = 1;
  std::optional<int> b;
  b = a;
  ASSERT_TRUE(b.has_value());
  ASSERT_EQ(*b, 1);
}

TEST(optional, copy_assignment_empty) {
  std::optional<int> a;
  std::optional<int> b = 5;
  b = a;
  ASSERT_FALSE(b.has_value());
}

TEST(optional, move_assignment_engaged) {
  std::optional<int> a = 42;
  std::optional<int> b;
  b = std::move(a);
  ASSERT_TRUE(b.has_value());
  ASSERT_EQ(*b, 42);
  ASSERT_FALSE(a.has_value());
}

TEST(optional, value_assignment) {
  std::optional<int> o;
  o = 100;
  ASSERT_TRUE(o.has_value());
  ASSERT_EQ(*o, 100);
  o = 200;
  ASSERT_EQ(*o, 200);
}

// ===========================================================================
// Accessors
// ===========================================================================

TEST(optional, dereference_operator) {
  std::optional<int> o = 55;
  ASSERT_EQ(*o, 55);
}

TEST(optional, arrow_operator) {
  struct Point {
    int x, y;
  };
  std::optional<Point> o = Point{3, 4};
  ASSERT_EQ(o->x, 3);
  ASSERT_EQ(o->y, 4);
}

TEST(optional, value_method) {
  std::optional<int> o = 7;
  ASSERT_EQ(o.value(), 7);
}

TEST(optional, value_or_engaged) {
  std::optional<int> o = 10;
  ASSERT_EQ(o.value_or(99), 10);
}

TEST(optional, value_or_empty) {
  std::optional<int> o;
  ASSERT_EQ(o.value_or(99), 99);
}

// ===========================================================================
// Modifiers
// ===========================================================================

TEST(optional, reset_disengages) {
  std::optional<int> o = 5;
  o.reset();
  ASSERT_FALSE(o.has_value());
}

TEST(optional, emplace_replaces_value) {
  std::optional<int> o = 1;
  o.emplace(42);
  ASSERT_TRUE(o.has_value());
  ASSERT_EQ(*o, 42);
}

TEST(optional, emplace_on_empty) {
  std::optional<int> o;
  o.emplace(7);
  ASSERT_TRUE(o.has_value());
  ASSERT_EQ(*o, 7);
}

TEST(optional, emplace_calls_destructor_first) {
  Tracked::instances = 0;
  std::optional<Tracked> o = Tracked(1);
  ASSERT_EQ(Tracked::instances, 1);
  o.emplace(2);
  // Old value destroyed, new one constructed.
  ASSERT_EQ(Tracked::instances, 1);
  ASSERT_EQ(o->value, 2);
}

TEST(optional, swap_both_engaged) {
  std::optional<int> a = 1, b = 2;
  a.swap(b);
  ASSERT_EQ(*a, 2);
  ASSERT_EQ(*b, 1);
}

TEST(optional, swap_first_empty) {
  std::optional<int> a, b = 9;
  a.swap(b);
  ASSERT_TRUE(a.has_value());
  ASSERT_EQ(*a, 9);
  ASSERT_FALSE(b.has_value());
}

TEST(optional, swap_second_empty) {
  std::optional<int> a = 3, b;
  a.swap(b);
  ASSERT_FALSE(a.has_value());
  ASSERT_TRUE(b.has_value());
  ASSERT_EQ(*b, 3);
}

TEST(optional, swap_both_empty) {
  std::optional<int> a, b;
  a.swap(b);
  ASSERT_FALSE(a.has_value());
  ASSERT_FALSE(b.has_value());
}

// ===========================================================================
// make_optional
// ===========================================================================

TEST(optional, make_optional_value) {
  auto o = std::make_optional(42);
  ASSERT_TRUE(o.has_value());
  ASSERT_EQ(*o, 42);
}

// ===========================================================================
// Comparison operators
// ===========================================================================

TEST(optional, equality_both_engaged_equal) {
  std::optional<int> a = 5, b = 5;
  ASSERT_TRUE(a == b);
  ASSERT_FALSE(a != b);
}

TEST(optional, equality_both_engaged_different) {
  std::optional<int> a = 5, b = 6;
  ASSERT_FALSE(a == b);
  ASSERT_TRUE(a != b);
}

TEST(optional, equality_both_empty) {
  std::optional<int> a, b;
  ASSERT_TRUE(a == b);
}

TEST(optional, equality_one_empty) {
  std::optional<int> a = 5, b;
  ASSERT_FALSE(a == b);
  ASSERT_TRUE(a != b);
}

TEST(optional, equality_with_nullopt) {
  std::optional<int> empty;
  std::optional<int> full = 1;
  ASSERT_TRUE(empty == std::nullopt);
  ASSERT_FALSE(full == std::nullopt);
  ASSERT_TRUE(std::nullopt == empty);
  ASSERT_FALSE(empty != std::nullopt);
  ASSERT_TRUE(full != std::nullopt);
}

TEST(optional, equality_with_value) {
  std::optional<int> o = 42;
  ASSERT_TRUE(o == 42);
  ASSERT_TRUE(42 == o);
  ASSERT_FALSE(o == 1);
  ASSERT_TRUE(o != 1);
}

TEST(optional, equality_empty_with_value) {
  std::optional<int> o;
  ASSERT_FALSE(o == 42);
  ASSERT_TRUE(o != 42);
}

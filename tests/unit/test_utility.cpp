#include <utility.h>

#include "../framework/test.h"

TEST(utility, move_produces_rvalue) {
  int x = 42;
  static_assert(std::is_rvalue_reference<decltype(std::move(x))>::value);
  int y = std::move(x);
  ASSERT_EQ(y, 42);
}

TEST(utility, move_class) {
  struct Tracker {
    bool moved = false;
    Tracker() = default;
    Tracker(Tracker&& other) noexcept { other.moved = true; }
  };
  Tracker a;
  Tracker b(std::move(a));
  ASSERT_TRUE(a.moved);
}

TEST(utility, forward_lvalue) {
  int x = 10;
  static_assert(std::is_lvalue_reference<decltype(std::forward<int&>(x))>::value);
  int& ref = std::forward<int&>(x);
  ASSERT_EQ(ref, 10);
  ref = 20;
  ASSERT_EQ(x, 20);
}

TEST(utility, forward_rvalue) {
  static_assert(std::is_rvalue_reference<decltype(std::forward<int>(42))>::value);
}

TEST(utility, swap_ints) {
  int a = 1, b = 2;
  std::swap(a, b);
  ASSERT_EQ(a, 2);
  ASSERT_EQ(b, 1);
}

TEST(utility, swap_self) {
  int x = 99;
  std::swap(x, x);
  ASSERT_EQ(x, 99);
}

TEST(utility, swap_array) {
  int a[3] = {1, 2, 3};
  int b[3] = {4, 5, 6};
  std::swap(a, b);
  ASSERT_EQ(a[0], 4);
  ASSERT_EQ(a[2], 6);
  ASSERT_EQ(b[0], 1);
  ASSERT_EQ(b[2], 3);
}

TEST(utility, swap_class) {
  struct Point {
    int x, y;
  };
  Point p = {1, 2}, q = {3, 4};
  std::swap(p, q);
  ASSERT_EQ(p.x, 3);
  ASSERT_EQ(p.y, 4);
  ASSERT_EQ(q.x, 1);
  ASSERT_EQ(q.y, 2);
}

TEST(utility, addressof) {
  int x = 42;
  ASSERT(std::addressof(x) == &x);
}

TEST(utility, addressof_const) {
  const int x = 10;
  ASSERT(std::addressof(x) == &x);
}

// Helper structs for void_t SFINAE test -- must be at file scope.
template <typename T, typename = void>
struct has_type_member : std::false_type {};
template <typename T>
struct has_type_member<T, std::void_t<typename T::type>> : std::true_type {};

struct WithTypeMember {
  using type = int;
};
struct WithoutTypeMember {};

TEST(utility, void_t_sfinae) {
  static_assert(has_type_member<WithTypeMember>::value);
  static_assert(!has_type_member<WithoutTypeMember>::value);
  ASSERT_TRUE(has_type_member<WithTypeMember>::value);
  ASSERT_FALSE(has_type_member<WithoutTypeMember>::value);
}

TEST(utility, is_signed_unsigned) {
  static_assert(std::is_signed_v<int>);
  static_assert(std::is_signed_v<signed char>);
  static_assert(!std::is_signed_v<unsigned int>);
  static_assert(std::is_unsigned_v<unsigned int>);
  static_assert(!std::is_unsigned_v<int>);
  ASSERT_TRUE(std::is_signed_v<int>);
  ASSERT_FALSE(std::is_signed_v<unsigned int>);
}

TEST(utility, is_trivially_destructible) {
  struct Trivial {
    int x;
  };
  struct NonTrivial {
    ~NonTrivial() {}
  };
  static_assert(std::is_trivially_destructible_v<Trivial>);
  static_assert(!std::is_trivially_destructible_v<NonTrivial>);
  ASSERT_TRUE(std::is_trivially_destructible_v<int>);
  ASSERT_FALSE(std::is_trivially_destructible_v<NonTrivial>);
}

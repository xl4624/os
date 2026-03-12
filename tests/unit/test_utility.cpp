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

// ===========================================================================
// Trivially traits
// ===========================================================================

TEST(utility, is_trivially_copyable) {
  struct Trivial {
    int x;
    float y;
  };
  struct NonTrivial {
    NonTrivial(const NonTrivial&) {}
  };
  static_assert(std::is_trivially_copyable_v<int>);
  static_assert(std::is_trivially_copyable_v<Trivial>);
  static_assert(!std::is_trivially_copyable_v<NonTrivial>);
  ASSERT_TRUE(std::is_trivially_copyable_v<int>);
  ASSERT_FALSE(std::is_trivially_copyable_v<NonTrivial>);
}

TEST(utility, is_trivially_constructible) {
  struct Trivial {
    int x;
  };
  struct NonTrivial {
    NonTrivial() : x(42) {}
    int x;
  };
  static_assert(std::is_trivially_constructible_v<int>);
  static_assert(std::is_trivially_constructible_v<Trivial>);
  static_assert(!std::is_trivially_constructible_v<NonTrivial>);
  ASSERT_TRUE(std::is_trivially_constructible_v<int>);
  ASSERT_FALSE(std::is_trivially_constructible_v<NonTrivial>);
}

TEST(utility, is_trivial) {
  struct Trivial {
    int x;
  };
  struct NonTrivial {
    NonTrivial() : x(0) {}
    int x;
  };
  static_assert(std::is_trivial_v<int>);
  static_assert(std::is_trivial_v<Trivial>);
  static_assert(!std::is_trivial_v<NonTrivial>);
  ASSERT_TRUE(std::is_trivial_v<int>);
  ASSERT_FALSE(std::is_trivial_v<NonTrivial>);
}

// ===========================================================================
// Layout / structural traits
// ===========================================================================

TEST(utility, is_standard_layout) {
  struct Standard {
    int x;
    int y;
  };
  struct NonStandard {
    virtual void f() {}
  };
  static_assert(std::is_standard_layout_v<int>);
  static_assert(std::is_standard_layout_v<Standard>);
  static_assert(!std::is_standard_layout_v<NonStandard>);
  ASSERT_TRUE(std::is_standard_layout_v<Standard>);
  ASSERT_FALSE(std::is_standard_layout_v<NonStandard>);
}

TEST(utility, is_pod) {
  struct Pod {
    int x;
    float y;
  };
  struct NonPod {
    virtual void f() {}
  };
  static_assert(std::is_pod_v<int>);
  static_assert(std::is_pod_v<Pod>);
  static_assert(!std::is_pod_v<NonPod>);
  ASSERT_TRUE(std::is_pod_v<Pod>);
  ASSERT_FALSE(std::is_pod_v<NonPod>);
}

TEST(utility, is_empty) {
  struct Empty {};
  struct NotEmpty {
    int x;
  };
  static_assert(std::is_empty_v<Empty>);
  static_assert(!std::is_empty_v<NotEmpty>);
  static_assert(!std::is_empty_v<int>);
  ASSERT_TRUE(std::is_empty_v<Empty>);
  ASSERT_FALSE(std::is_empty_v<NotEmpty>);
}

TEST(utility, is_final) {
  struct Base {};
  struct Final final : Base {};
  struct NotFinal : Base {};
  static_assert(std::is_final_v<Final>);
  static_assert(!std::is_final_v<NotFinal>);
  static_assert(!std::is_final_v<int>);
  ASSERT_TRUE(std::is_final_v<Final>);
  ASSERT_FALSE(std::is_final_v<NotFinal>);
}

TEST(utility, is_aggregate) {
  struct Aggregate {
    int x;
    int y;
  };
  struct NonAggregate {
    explicit NonAggregate(int v) : x(v) {}
    int x;
  };
  static_assert(std::is_aggregate_v<Aggregate>);
  static_assert(!std::is_aggregate_v<NonAggregate>);
  ASSERT_TRUE(std::is_aggregate_v<Aggregate>);
  ASSERT_FALSE(std::is_aggregate_v<NonAggregate>);
}

// ===========================================================================
// Nothrow constructibility
// ===========================================================================

TEST(utility, is_nothrow_default_constructible) {
  struct Nothrow {
    Nothrow() noexcept {}
  };
  struct Throwing {
    Throwing() {}
  };
  static_assert(std::is_nothrow_default_constructible_v<int>);
  static_assert(std::is_nothrow_default_constructible_v<Nothrow>);
  static_assert(!std::is_nothrow_default_constructible_v<Throwing>);
  ASSERT_TRUE(std::is_nothrow_default_constructible_v<int>);
  ASSERT_TRUE(std::is_nothrow_default_constructible_v<Nothrow>);
  ASSERT_FALSE(std::is_nothrow_default_constructible_v<Throwing>);
}

TEST(utility, is_nothrow_copy_constructible) {
  struct Nothrow {
    Nothrow() = default;
    Nothrow(const Nothrow&) noexcept {}
  };
  struct Throwing {
    Throwing() = default;
    Throwing(const Throwing&) {}
  };
  static_assert(std::is_nothrow_copy_constructible_v<int>);
  static_assert(std::is_nothrow_copy_constructible_v<Nothrow>);
  static_assert(!std::is_nothrow_copy_constructible_v<Throwing>);
  ASSERT_TRUE(std::is_nothrow_copy_constructible_v<int>);
  ASSERT_FALSE(std::is_nothrow_copy_constructible_v<Throwing>);
}

TEST(utility, is_nothrow_move_constructible) {
  struct Nothrow {
    Nothrow() = default;
    Nothrow(Nothrow&&) noexcept {}
  };
  struct Throwing {
    Throwing() = default;
    Throwing(Throwing&&) {}
  };
  static_assert(std::is_nothrow_move_constructible_v<int>);
  static_assert(std::is_nothrow_move_constructible_v<Nothrow>);
  static_assert(!std::is_nothrow_move_constructible_v<Throwing>);
  ASSERT_TRUE(std::is_nothrow_move_constructible_v<int>);
  ASSERT_FALSE(std::is_nothrow_move_constructible_v<Throwing>);
}

// ===========================================================================
// Nothrow swappable
// ===========================================================================

struct NothrowSwappable {
  friend void swap(NothrowSwappable&, NothrowSwappable&) noexcept {}
};

TEST(utility, is_nothrow_swappable) {
  static_assert(std::is_nothrow_swappable_v<int>);
  static_assert(std::is_nothrow_swappable_v<NothrowSwappable>);
  ASSERT_TRUE(std::is_nothrow_swappable_v<int>);
  ASSERT_TRUE(std::is_nothrow_swappable_v<NothrowSwappable>);
}

// ===========================================================================
// invoke_result / result_of
// ===========================================================================

struct ReturnsInt {
  int operator()() noexcept { return 42; }
};

struct TakesIntReturnsFloat {
  float operator()(int) noexcept { return 1.0f; }
};

TEST(utility, invoke_result_nullary) {
  static_assert(std::is_same_v<std::invoke_result_t<ReturnsInt>, int>);
  ASSERT_TRUE((std::is_same_v<std::invoke_result_t<ReturnsInt>, int>));
}

TEST(utility, result_of_nullary) {
  static_assert(std::is_same_v<std::result_of_t<ReturnsInt()>, int>);
  ASSERT_TRUE((std::is_same_v<std::result_of_t<ReturnsInt()>, int>));
}

TEST(utility, invoke_result_with_arg) {
  static_assert(std::is_same_v<std::invoke_result_t<TakesIntReturnsFloat(int)>, float>);
  ASSERT_TRUE((std::is_same_v<std::invoke_result_t<TakesIntReturnsFloat(int)>, float>));
}

TEST(utility, result_of_with_arg) {
  static_assert(std::is_same_v<std::result_of_t<TakesIntReturnsFloat(int)>, float>);
  ASSERT_TRUE((std::is_same_v<std::result_of_t<TakesIntReturnsFloat(int)>, float>));
}

// ===========================================================================
// is_invocable
// ===========================================================================

TEST(utility, is_invocable) {
  struct Callable {
    void operator()() noexcept {}
  };
  struct NotNoexcept {
    void operator()() {}
  };
  static_assert(std::is_invocable_v<Callable>);
  static_assert(!std::is_invocable_v<NotNoexcept>);
  ASSERT_TRUE(std::is_invocable_v<Callable>);
  ASSERT_FALSE(std::is_invocable_v<NotNoexcept>);
}

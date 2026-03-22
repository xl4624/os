#include <functional.h>

#include "../framework/test.h"

namespace {

int free_add(int a, int b) { return a + b; }

struct Adder {
  int base;
  int operator()(int x) const { return base + x; }
};

}  // namespace

// ===========================================================================
// Construction and null state
// ===========================================================================

TEST(function, default_is_empty) {
  std::function<int()> f;
  ASSERT_FALSE(static_cast<bool>(f));
}

TEST(function, nullptr_construct) {
  std::function<int()> f(nullptr);
  ASSERT_FALSE(static_cast<bool>(f));
}

// ===========================================================================
// Wrapping different callable types
// ===========================================================================

TEST(function, wrap_free_function) {
  std::function<int(int, int)> f(free_add);
  ASSERT_TRUE(static_cast<bool>(f));
  ASSERT_EQ(f(3, 4), 7);
}

TEST(function, wrap_lambda) {
  std::function<int(int)> f([](int x) { return x * 2; });
  ASSERT_EQ(f(5), 10);
}

TEST(function, wrap_lambda_with_capture) {
  int factor = 3;
  std::function<int(int)> f([factor](int x) { return x * factor; });
  ASSERT_EQ(f(4), 12);
}

TEST(function, wrap_functor) {
  Adder adder{10};
  std::function<int(int)> f(adder);
  ASSERT_EQ(f(5), 15);
}

// ===========================================================================
// Copy and move
// ===========================================================================

TEST(function, copy_construct) {
  std::function<int(int)> f([](int x) { return x + 1; });
  std::function<int(int)> g(f);
  ASSERT_EQ(f(10), 11);
  ASSERT_EQ(g(10), 11);
}

TEST(function, move_construct) {
  std::function<int(int)> f([](int x) { return x + 1; });
  std::function<int(int)> g(std::move(f));
  ASSERT_EQ(g(10), 11);
  ASSERT_FALSE(static_cast<bool>(f));  // NOLINT(bugprone-use-after-move)
}

TEST(function, copy_assign) {
  std::function<int()> f([]() { return 42; });
  std::function<int()> g;
  g = f;
  ASSERT_EQ(f(), 42);
  ASSERT_EQ(g(), 42);
}

TEST(function, move_assign) {
  std::function<int()> f([]() { return 99; });
  std::function<int()> g;
  g = std::move(f);
  ASSERT_EQ(g(), 99);
  ASSERT_FALSE(static_cast<bool>(f));  // NOLINT(bugprone-use-after-move)
}

TEST(function, assign_nullptr) {
  std::function<int()> f([]() { return 1; });
  ASSERT_TRUE(static_cast<bool>(f));
  f = nullptr;
  ASSERT_FALSE(static_cast<bool>(f));
}

// ===========================================================================
// Large callable (heap fallback)
// ===========================================================================

TEST(function, large_capture_lambda) {
  int a = 1, b = 2, c = 3, d = 4, e = 5;
  int f_val = 6, g = 7, h = 8, i = 9, j = 10;
  std::function<int()> fn([=]() { return a + b + c + d + e + f_val + g + h + i + j; });
  ASSERT_EQ(fn(), 55);
}

TEST(function, copy_large_capture) {
  int a = 1, b = 2, c = 3, d = 4, e = 5;
  int f_val = 6, g = 7, h = 8, i = 9, j = 10;
  std::function<int()> fn([=]() { return a + b + c + d + e + f_val + g + h + i + j; });
  std::function<int()> fn2(fn);
  ASSERT_EQ(fn(), 55);
  ASSERT_EQ(fn2(), 55);
}

// ===========================================================================
// Reassignment to different callable
// ===========================================================================

TEST(function, reassign_different_callable) {
  std::function<int(int)> f([](int x) { return x + 1; });
  ASSERT_EQ(f(5), 6);
  f = [](int x) { return x * 2; };
  ASSERT_EQ(f(5), 10);
}

#include <unique_ptr.h>

#include "../framework/test.h"

namespace {

// Tracks destructor calls via a shared counter.
struct Widget {
  static int destroy_count;
  int value = 0;
  Widget() = default;
  explicit Widget(int v) : value(v) {}
  ~Widget() { ++destroy_count; }
};

int Widget::destroy_count = 0;

struct Base {
  virtual ~Base() = default;
  int x = 0;
};

struct Derived : Base {
  int y = 0;
};

}  // namespace

// ===========================================================================
// Construction and null state
// ===========================================================================

TEST(unique_ptr, default_is_null) {
  std::unique_ptr<int> p;
  ASSERT_NULL(p.get());
  ASSERT_FALSE(static_cast<bool>(p));
}

TEST(unique_ptr, nullptr_construction) {
  std::unique_ptr<int> p(nullptr);
  ASSERT_NULL(p.get());
}

TEST(unique_ptr, pointer_construction) {
  std::unique_ptr<int> p(new int(42));
  ASSERT_NOT_NULL(p.get());
  ASSERT_EQ(*p, 42);
  ASSERT_TRUE(static_cast<bool>(p));
}

// ===========================================================================
// Destructor
// ===========================================================================

TEST(unique_ptr, destructor_frees_on_scope_exit) {
  Widget::destroy_count = 0;
  {
    std::unique_ptr<Widget> p(new Widget(1));
    ASSERT_EQ(Widget::destroy_count, 0);
  }
  ASSERT_EQ(Widget::destroy_count, 1);
}

TEST(unique_ptr, destructor_frees_on_reset) {
  Widget::destroy_count = 0;
  std::unique_ptr<Widget> p(new Widget(2));
  p.reset();
  ASSERT_EQ(Widget::destroy_count, 1);
  ASSERT_NULL(p.get());
}

TEST(unique_ptr, no_double_free_on_null) {
  Widget::destroy_count = 0;
  {
    std::unique_ptr<Widget> p;
  }
  ASSERT_EQ(Widget::destroy_count, 0);
}

// ===========================================================================
// Move semantics
// ===========================================================================

TEST(unique_ptr, move_construction_transfers_ownership) {
  std::unique_ptr<int> a(new int(7));
  int* raw = a.get();
  std::unique_ptr<int> b(std::move(a));
  ASSERT_NULL(a.get());
  ASSERT_EQ(b.get(), raw);
  ASSERT_EQ(*b, 7);
}

TEST(unique_ptr, move_assignment_transfers_ownership) {
  Widget::destroy_count = 0;
  std::unique_ptr<Widget> a(new Widget(3));
  std::unique_ptr<Widget> b(new Widget(4));
  // Moving a into b should destroy b's original widget.
  b = std::move(a);
  ASSERT_EQ(Widget::destroy_count, 1);
  ASSERT_NULL(a.get());
  ASSERT_EQ(b->value, 3);
}

TEST(unique_ptr, move_assign_nullptr_destroys) {
  Widget::destroy_count = 0;
  std::unique_ptr<Widget> p(new Widget(5));
  p = nullptr;
  ASSERT_EQ(Widget::destroy_count, 1);
  ASSERT_NULL(p.get());
}

// ===========================================================================
// release and reset
// ===========================================================================

TEST(unique_ptr, release_returns_raw_pointer) {
  std::unique_ptr<int> p(new int(99));
  int* raw = p.release();
  ASSERT_NULL(p.get());
  ASSERT_EQ(*raw, 99);
  delete raw;
}

TEST(unique_ptr, reset_to_new_value) {
  Widget::destroy_count = 0;
  std::unique_ptr<Widget> p(new Widget(1));
  p.reset(new Widget(2));
  ASSERT_EQ(Widget::destroy_count, 1);
  ASSERT_EQ(p->value, 2);
}

// ===========================================================================
// Dereference and member access
// ===========================================================================

TEST(unique_ptr, dereference_operator) {
  std::unique_ptr<int> p(new int(55));
  ASSERT_EQ(*p, 55);
  *p = 66;
  ASSERT_EQ(*p, 66);
}

TEST(unique_ptr, arrow_operator) {
  struct Point {
    int x, y;
  };
  std::unique_ptr<Point> p(new Point{3, 4});
  ASSERT_EQ(p->x, 3);
  ASSERT_EQ(p->y, 4);
  p->x = 10;
  ASSERT_EQ(p->x, 10);
}

// ===========================================================================
// swap
// ===========================================================================

TEST(unique_ptr, swap) {
  std::unique_ptr<int> a(new int(1));
  std::unique_ptr<int> b(new int(2));
  int* raw_a = a.get();
  int* raw_b = b.get();
  a.swap(b);
  ASSERT_EQ(a.get(), raw_b);
  ASSERT_EQ(b.get(), raw_a);
}

// ===========================================================================
// Comparison operators
// ===========================================================================

TEST(unique_ptr, equality_two_ptrs) {
  std::unique_ptr<int> a(new int(1));
  std::unique_ptr<int> b(new int(2));
  ASSERT_FALSE(a == b);
  ASSERT_TRUE(a != b);
}

TEST(unique_ptr, null_comparison) {
  std::unique_ptr<int> p;
  ASSERT_TRUE(p == nullptr);
  ASSERT_TRUE(nullptr == p);
  ASSERT_FALSE(p != nullptr);

  std::unique_ptr<int> q(new int(1));
  ASSERT_FALSE(q == nullptr);
  ASSERT_TRUE(q != nullptr);
}

// ===========================================================================
// make_unique for single objects
// ===========================================================================

TEST(unique_ptr, make_unique_default) {
  auto p = std::make_unique<int>();
  ASSERT_NOT_NULL(p.get());
}

TEST(unique_ptr, make_unique_with_args) {
  auto p = std::make_unique<int>(42);
  ASSERT_EQ(*p, 42);
}

TEST(unique_ptr, make_unique_struct) {
  struct Point {
    int x, y;
    Point(int a, int b) : x(a), y(b) {}
  };
  auto p = std::make_unique<Point>(3, 4);
  ASSERT_EQ(p->x, 3);
  ASSERT_EQ(p->y, 4);
}

TEST(unique_ptr, make_unique_destructor_runs) {
  Widget::destroy_count = 0;
  {
    auto p = std::make_unique<Widget>(10);
    ASSERT_EQ(Widget::destroy_count, 0);
  }
  ASSERT_EQ(Widget::destroy_count, 1);
}

// ===========================================================================
// Array specialization
// ===========================================================================

TEST(unique_ptr, array_make_unique) {
  auto p = std::make_unique<int[]>(5);
  ASSERT_NOT_NULL(p.get());
  for (int i = 0; i < 5; ++i) p[i] = i;
  for (int i = 0; i < 5; ++i) ASSERT_EQ(p[i], i);
}

TEST(unique_ptr, array_move_construction) {
  auto a = std::make_unique<int[]>(3);
  a[0] = 10;
  int* raw = a.get();
  auto b = std::move(a);
  ASSERT_NULL(a.get());
  ASSERT_EQ(b.get(), raw);
  ASSERT_EQ(b[0], 10);
}

TEST(unique_ptr, array_destructor_calls_delete_array) {
  Widget::destroy_count = 0;
  {
    auto p = std::make_unique<Widget[]>(3);
    ASSERT_EQ(Widget::destroy_count, 0);
  }
  // delete[] calls destructor for each element.
  ASSERT_EQ(Widget::destroy_count, 3);
}

// ===========================================================================
// Custom deleter
// ===========================================================================

TEST(unique_ptr, custom_deleter) {
  int freed = 0;
  auto deleter = [&](int* p) {
    ++freed;
    delete p;
  };
  {
    std::unique_ptr<int, decltype(deleter)> p(new int(1), deleter);
    ASSERT_EQ(freed, 0);
  }
  ASSERT_EQ(freed, 1);
}

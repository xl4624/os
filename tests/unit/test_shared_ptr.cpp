#include <memory.h>

#include "../framework/test.h"

namespace {

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

TEST(shared_ptr, default_is_null) {
  std::shared_ptr<int> p;
  ASSERT_NULL(p.get());
  ASSERT_FALSE(static_cast<bool>(p));
  ASSERT_EQ(p.use_count(), 0U);
}

TEST(shared_ptr, nullptr_construct) {
  std::shared_ptr<int> p(nullptr);
  ASSERT_NULL(p.get());
  ASSERT_EQ(p.use_count(), 0U);
}

TEST(shared_ptr, pointer_construct) {
  std::shared_ptr<int> p(new int(42));
  ASSERT_NOT_NULL(p.get());
  ASSERT_EQ(*p, 42);
  ASSERT_EQ(p.use_count(), 1U);
}

// ===========================================================================
// make_shared
// ===========================================================================

TEST(shared_ptr, make_shared) {
  auto p = std::make_shared<int>(99);
  ASSERT_EQ(*p, 99);
  ASSERT_EQ(p.use_count(), 1U);
  ASSERT_TRUE(p.unique());
}

TEST(shared_ptr, make_shared_struct) {
  auto p = std::make_shared<Widget>(7);
  ASSERT_EQ(p->value, 7);
}

// ===========================================================================
// Reference counting
// ===========================================================================

TEST(shared_ptr, copy_increments_refcount) {
  auto p = std::make_shared<int>(10);
  ASSERT_EQ(p.use_count(), 1U);
  {
    std::shared_ptr<int> q(p);
    ASSERT_EQ(p.use_count(), 2U);
    ASSERT_EQ(q.use_count(), 2U);
    ASSERT_EQ(p.get(), q.get());
  }
  ASSERT_EQ(p.use_count(), 1U);
}

TEST(shared_ptr, copy_assign_refcount) {
  auto a = std::make_shared<int>(1);
  auto b = std::make_shared<int>(2);
  ASSERT_EQ(a.use_count(), 1U);
  b = a;
  ASSERT_EQ(a.use_count(), 2U);
  ASSERT_EQ(*b, 1);
}

TEST(shared_ptr, destructor_decrements_refcount) {
  Widget::destroy_count = 0;
  {
    auto p = std::make_shared<Widget>(5);
    {
      std::shared_ptr<Widget> q(p);
      ASSERT_EQ(Widget::destroy_count, 0);
    }
    ASSERT_EQ(Widget::destroy_count, 0);
  }
  ASSERT_EQ(Widget::destroy_count, 1);
}

// ===========================================================================
// Move
// ===========================================================================

TEST(shared_ptr, move_construct) {
  auto a = std::make_shared<int>(42);
  int* raw = a.get();
  std::shared_ptr<int> b(std::move(a));
  ASSERT_NULL(a.get());          // NOLINT(bugprone-use-after-move)
  ASSERT_EQ(a.use_count(), 0U);  // NOLINT(bugprone-use-after-move)
  ASSERT_EQ(b.get(), raw);
  ASSERT_EQ(b.use_count(), 1U);
}

TEST(shared_ptr, move_assign) {
  auto a = std::make_shared<int>(10);
  std::shared_ptr<int> b;
  b = std::move(a);
  ASSERT_EQ(*b, 10);
  ASSERT_EQ(b.use_count(), 1U);
  ASSERT_NULL(a.get());  // NOLINT(bugprone-use-after-move)
}

// ===========================================================================
// reset
// ===========================================================================

TEST(shared_ptr, reset_to_null) {
  Widget::destroy_count = 0;
  auto p = std::make_shared<Widget>(1);
  p.reset();
  ASSERT_NULL(p.get());
  ASSERT_EQ(p.use_count(), 0U);
  ASSERT_EQ(Widget::destroy_count, 1);
}

TEST(shared_ptr, reset_to_new_pointer) {
  Widget::destroy_count = 0;
  auto p = std::make_shared<Widget>(1);
  p.reset(new Widget(2));
  ASSERT_EQ(p->value, 2);
  ASSERT_EQ(p.use_count(), 1U);
  ASSERT_EQ(Widget::destroy_count, 1);
}

// ===========================================================================
// Shared ownership keeps object alive
// ===========================================================================

TEST(shared_ptr, shared_ownership) {
  Widget::destroy_count = 0;
  std::shared_ptr<Widget> outer;
  {
    auto inner = std::make_shared<Widget>(42);
    outer = inner;
    ASSERT_EQ(outer.use_count(), 2U);
  }
  // inner is gone but outer still holds a ref.
  ASSERT_EQ(Widget::destroy_count, 0);
  ASSERT_EQ(outer->value, 42);
  ASSERT_EQ(outer.use_count(), 1U);
}

// ===========================================================================
// Comparison
// ===========================================================================

TEST(shared_ptr, comparison) {
  auto a = std::make_shared<int>(1);
  auto b = a;
  auto c = std::make_shared<int>(1);
  ASSERT_TRUE(a == b);
  ASSERT_TRUE(a != c);
}

TEST(shared_ptr, comparison_nullptr) {
  std::shared_ptr<int> null;
  auto p = std::make_shared<int>(1);
  ASSERT_TRUE(null == nullptr);
  ASSERT_TRUE(nullptr == null);
  ASSERT_TRUE(p != nullptr);
  ASSERT_TRUE(nullptr != p);
}

// ===========================================================================
// Converting constructor (derived to base)
// ===========================================================================

TEST(shared_ptr, derived_to_base) {
  auto d = std::make_shared<Derived>();
  d->x = 10;
  d->y = 20;
  std::shared_ptr<Base> b(d);
  ASSERT_EQ(b->x, 10);
  ASSERT_EQ(b.use_count(), 2U);
}

// ===========================================================================
// assign nullptr
// ===========================================================================

TEST(shared_ptr, assign_nullptr) {
  auto p = std::make_shared<int>(1);
  p = nullptr;
  ASSERT_NULL(p.get());
  ASSERT_EQ(p.use_count(), 0U);
}

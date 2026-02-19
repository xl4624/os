#include <utility.h>

#include "../framework/test.h"

TEST(type_traits, integral_constant) {
    static_assert(std::integral_constant<int, 42>::value == 42);
    static_assert(std::true_type::value == true);
    static_assert(std::false_type::value == false);
    ASSERT_TRUE(std::true_type::value);
    ASSERT_FALSE(std::false_type::value);
    // operator() and operator T() round-trip
    constexpr int val = std::integral_constant<int, 7>{}();
    ASSERT_EQ(val, 7);
}

TEST(type_traits, is_same) {
    static_assert(std::is_same<int, int>::value);
    static_assert(!std::is_same<int, float>::value);
    static_assert(!std::is_same_v<int, const int>);
    ASSERT_TRUE((std::is_same<int, int>::value));
    ASSERT_FALSE((std::is_same<int, float>::value));
    ASSERT_FALSE((std::is_same_v<int, const int>));
}

TEST(type_traits, is_integral) {
    static_assert(std::is_integral_v<bool>);
    static_assert(std::is_integral_v<char>);
    static_assert(std::is_integral_v<int>);
    static_assert(std::is_integral_v<unsigned long>);
    static_assert(!std::is_integral_v<float>);
    static_assert(!std::is_integral_v<double>);
    static_assert(!std::is_integral_v<int *>);
    ASSERT_TRUE(std::is_integral_v<int>);
    ASSERT_FALSE(std::is_integral_v<float>);
}

TEST(type_traits, is_floating_point) {
    static_assert(std::is_floating_point_v<float>);
    static_assert(std::is_floating_point_v<double>);
    static_assert(std::is_floating_point_v<long double>);
    static_assert(!std::is_floating_point_v<int>);
    ASSERT_TRUE(std::is_floating_point_v<double>);
    ASSERT_FALSE(std::is_floating_point_v<int>);
}

TEST(type_traits, is_void) {
    static_assert(std::is_void_v<void>);
    static_assert(!std::is_void_v<int>);
    static_assert(!std::is_void_v<void *>);
    ASSERT_TRUE(std::is_void_v<void>);
    ASSERT_FALSE(std::is_void_v<int>);
}

TEST(type_traits, is_pointer) {
    static_assert(std::is_pointer_v<int *>);
    static_assert(std::is_pointer_v<void *>);
    static_assert(std::is_pointer_v<const int *>);
    static_assert(!std::is_pointer_v<int>);
    static_assert(!std::is_pointer_v<int &>);
    ASSERT_TRUE(std::is_pointer_v<int *>);
    ASSERT_FALSE(std::is_pointer_v<int>);
}

TEST(type_traits, is_reference) {
    static_assert(std::is_reference_v<int &>);
    static_assert(std::is_reference_v<int &&>);
    static_assert(!std::is_reference_v<int>);
    static_assert(std::is_lvalue_reference_v<int &>);
    static_assert(!std::is_lvalue_reference_v<int &&>);
    static_assert(std::is_rvalue_reference_v<int &&>);
    static_assert(!std::is_rvalue_reference_v<int &>);
    ASSERT_TRUE(std::is_lvalue_reference_v<int &>);
    ASSERT_TRUE(std::is_rvalue_reference_v<int &&>);
}

TEST(type_traits, is_array) {
    static_assert(std::is_array_v<int[]>);
    static_assert(std::is_array_v<int[5]>);
    static_assert(!std::is_array_v<int>);
    static_assert(!std::is_array_v<int *>);
    ASSERT_TRUE(std::is_array_v<int[3]>);
    ASSERT_FALSE(std::is_array_v<int>);
}

TEST(type_traits, is_bounded_unbounded_array) {
    static_assert(std::is_bounded_array_v<int[5]>);
    static_assert(!std::is_bounded_array_v<int[]>);
    static_assert(!std::is_bounded_array_v<int>);
    static_assert(std::is_unbounded_array_v<int[]>);
    static_assert(!std::is_unbounded_array_v<int[5]>);
    ASSERT_TRUE(std::is_bounded_array_v<int[5]>);
    ASSERT_FALSE(std::is_bounded_array_v<int[]>);
}

TEST(type_traits, is_const_volatile) {
    static_assert(std::is_const_v<const int>);
    static_assert(!std::is_const_v<int>);
    static_assert(std::is_volatile_v<volatile int>);
    static_assert(!std::is_volatile_v<int>);
    ASSERT_TRUE(std::is_const_v<const int>);
    ASSERT_FALSE(std::is_const_v<int>);
}

TEST(type_traits, remove_cv) {
    static_assert(std::is_same_v<std::remove_const_t<const int>, int>);
    static_assert(std::is_same_v<std::remove_const_t<int>, int>);
    static_assert(std::is_same_v<std::remove_volatile_t<volatile int>, int>);
    static_assert(std::is_same_v<std::remove_cv_t<const volatile int>, int>);
    ASSERT_TRUE((std::is_same_v<std::remove_const_t<const int>, int>));
    ASSERT_TRUE((std::is_same_v<std::remove_cv_t<const volatile int>, int>));
}

TEST(type_traits, remove_add_reference) {
    static_assert(std::is_same_v<std::remove_reference_t<int &>, int>);
    static_assert(std::is_same_v<std::remove_reference_t<int &&>, int>);
    static_assert(std::is_same_v<std::remove_reference_t<int>, int>);
    static_assert(std::is_same_v<std::add_lvalue_reference_t<int>, int &>);
    static_assert(std::is_same_v<std::add_rvalue_reference_t<int>, int &&>);
    ASSERT_TRUE((std::is_same_v<std::remove_reference_t<int &>, int>));
    ASSERT_TRUE((std::is_same_v<std::add_lvalue_reference_t<int>, int &>));
}

TEST(type_traits, remove_add_pointer) {
    static_assert(std::is_same_v<std::remove_pointer_t<int *>, int>);
    static_assert(std::is_same_v<std::remove_pointer_t<int>, int>);
    static_assert(std::is_same_v<std::add_pointer_t<int>, int *>);
    static_assert(std::is_same_v<std::add_pointer_t<const int>, const int *>);
    ASSERT_TRUE((std::is_same_v<std::remove_pointer_t<int *>, int>));
    ASSERT_TRUE((std::is_same_v<std::add_pointer_t<int>, int *>));
}

TEST(type_traits, conditional) {
    static_assert(std::is_same_v<std::conditional_t<true, int, float>, int>);
    static_assert(std::is_same_v<std::conditional_t<false, int, float>, float>);
    ASSERT_TRUE((std::is_same_v<std::conditional_t<true, int, float>, int>));
    ASSERT_TRUE((std::is_same_v<std::conditional_t<false, int, float>, float>));
}

TEST(type_traits, enable_if) {
    static_assert(std::is_same_v<std::enable_if_t<true, int>, int>);
    static_assert(std::is_same_v<std::enable_if_t<true>, void>);
    ASSERT_TRUE((std::is_same_v<std::enable_if_t<true, int>, int>));
}

TEST(type_traits, decay) {
    static_assert(std::is_same_v<std::decay_t<int>, int>);
    static_assert(std::is_same_v<std::decay_t<const int>, int>);
    static_assert(std::is_same_v<std::decay_t<int &>, int>);
    static_assert(std::is_same_v<std::decay_t<int &&>, int>);
    static_assert(std::is_same_v<std::decay_t<int[5]>, int *>);
    ASSERT_TRUE((std::is_same_v<std::decay_t<const int &>, int>));
    ASSERT_TRUE((std::is_same_v<std::decay_t<int[3]>, int *>));
}

TEST(type_traits, is_enum) {
    enum Color { Red, Green, Blue };
    enum class Status : int { Ok, Fail };
    static_assert(std::is_enum_v<Color>);
    static_assert(std::is_enum_v<Status>);
    static_assert(!std::is_enum_v<int>);
    static_assert(std::is_same_v<std::underlying_type_t<Status>, int>);
    ASSERT_TRUE(std::is_enum_v<Color>);
    ASSERT_FALSE(std::is_enum_v<int>);
}

TEST(type_traits, is_class) {
    struct Foo {};
    union Bar {
        int a;
        float b;
    };
    static_assert(std::is_class_v<Foo>);
    static_assert(!std::is_class_v<Bar>);
    static_assert(!std::is_class_v<int>);
    ASSERT_TRUE(std::is_class_v<Foo>);
    ASSERT_FALSE(std::is_class_v<int>);
}

TEST(type_traits, is_base_of) {
    struct Base {};
    struct Derived : Base {};
    struct Unrelated {};
    static_assert(std::is_base_of_v<Base, Derived>);
    static_assert(!std::is_base_of_v<Derived, Base>);
    static_assert(!std::is_base_of_v<Base, Unrelated>);
    ASSERT_TRUE((std::is_base_of_v<Base, Derived>));
    ASSERT_FALSE((std::is_base_of_v<Derived, Base>));
}

TEST(type_traits, make_unsigned) {
    static_assert(std::is_same_v<std::make_unsigned_t<int>, unsigned int>);
    static_assert(std::is_same_v<std::make_unsigned_t<char>, unsigned char>);
    static_assert(std::is_same_v<std::make_unsigned_t<short>, unsigned short>);
    ASSERT_TRUE((std::is_same_v<std::make_unsigned_t<int>, unsigned int>));
}

TEST(type_traits, make_signed) {
    static_assert(std::is_same_v<std::make_signed_t<unsigned int>, int>);
    static_assert(
        std::is_same_v<std::make_signed_t<unsigned char>, signed char>);
    ASSERT_TRUE((std::is_same_v<std::make_signed_t<unsigned int>, int>));
}

TEST(type_traits, remove_extent) {
    static_assert(std::is_same_v<std::remove_extent_t<int[5]>, int>);
    static_assert(std::is_same_v<std::remove_extent_t<int[]>, int>);
    static_assert(std::is_same_v<std::remove_extent_t<int[2][3]>, int[3]>);
    static_assert(std::is_same_v<std::remove_extent_t<int>, int>);
    ASSERT_TRUE((std::is_same_v<std::remove_extent_t<int[5]>, int>));
}

TEST(type_traits, rank_and_extent) {
    static_assert(std::rank_v<int> == 0);
    static_assert(std::rank_v<int[5]> == 1);
    static_assert(std::rank_v<int[2][3]> == 2);
    static_assert(std::extent_v<int[5]> == 5);
    static_assert(std::extent_v<int[2][3]> == 2);
    static_assert(std::extent_v<int[2][3], 1> == 3);
    ASSERT_EQ(std::rank_v<int[2][3]>, 2u);
    ASSERT_EQ(std::extent_v<int[4]>, 4u);
}

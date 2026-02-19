#include <array.h>

#include "../framework/test.h"

TEST(array, element_access) {
    std::array<int, 3> a = {1, 2, 3};
    ASSERT_EQ(a[0], 1);
    ASSERT_EQ(a[1], 2);
    ASSERT_EQ(a[2], 3);
    ASSERT_EQ(a.at(0), 1);
    ASSERT_EQ(a.front(), 1);
    ASSERT_EQ(a.back(), 3);
}

TEST(array, const_element_access) {
    const std::array<int, 3> a = {10, 20, 30};
    ASSERT_EQ(a[0], 10);
    ASSERT_EQ(a.front(), 10);
    ASSERT_EQ(a.back(), 30);
    ASSERT_EQ(a.at(1), 20);
}

TEST(array, size_and_empty) {
    std::array<int, 5> a = {};
    ASSERT_EQ(a.size(), 5u);
    ASSERT_EQ(a.max_size(), 5u);
    ASSERT_FALSE(a.empty());
}

TEST(array, zero_size) {
    std::array<int, 0> a = {};
    ASSERT_EQ(a.size(), 0u);
    ASSERT_TRUE(a.empty());
    ASSERT_NULL(a.data());
    ASSERT_NULL(a.begin());
    ASSERT_NULL(a.end());
}

TEST(array, data_pointer) {
    std::array<int, 3> a = {10, 20, 30};
    int *p = a.data();
    ASSERT_NOT_NULL(p);
    ASSERT_EQ(p[0], 10);
    ASSERT_EQ(p[2], 30);
    // data() must point to the first element
    ASSERT(p == &a[0]);
}

TEST(array, iterator_loop) {
    std::array<int, 4> a = {1, 2, 3, 4};
    int sum = 0;
    for (auto it = a.begin(); it != a.end(); ++it) {
        sum += *it;
    }
    ASSERT_EQ(sum, 10);
}

TEST(array, range_based_for) {
    std::array<int, 4> a = {1, 2, 3, 4};
    int sum = 0;
    for (int v : a) {
        sum += v;
    }
    ASSERT_EQ(sum, 10);
}

TEST(array, cbegin_cend) {
    std::array<int, 3> a = {5, 6, 7};
    int sum = 0;
    for (auto it = a.cbegin(); it != a.cend(); ++it) {
        sum += *it;
    }
    ASSERT_EQ(sum, 18);
}

TEST(array, fill) {
    std::array<int, 5> a = {};
    a.fill(7);
    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(a[i], 7);
    }
}

TEST(array, fill_overwrites) {
    std::array<int, 3> a = {1, 2, 3};
    a.fill(0);
    ASSERT_EQ(a[0], 0);
    ASSERT_EQ(a[1], 0);
    ASSERT_EQ(a[2], 0);
}

TEST(array, swap) {
    std::array<int, 3> a = {1, 2, 3};
    std::array<int, 3> b = {4, 5, 6};
    a.swap(b);
    ASSERT_EQ(a[0], 4);
    ASSERT_EQ(a[1], 5);
    ASSERT_EQ(a[2], 6);
    ASSERT_EQ(b[0], 1);
    ASSERT_EQ(b[1], 2);
    ASSERT_EQ(b[2], 3);
}

TEST(array, aggregate_init_partial) {
    // Elements not explicitly initialised must be value-initialised to zero.
    std::array<int, 5> a = {1, 2};
    ASSERT_EQ(a[0], 1);
    ASSERT_EQ(a[1], 2);
    ASSERT_EQ(a[2], 0);
    ASSERT_EQ(a[3], 0);
    ASSERT_EQ(a[4], 0);
}

TEST(array, write_through_index) {
    std::array<int, 3> a = {0, 0, 0};
    a[1] = 42;
    ASSERT_EQ(a[1], 42);
    ASSERT_EQ(a[0], 0);
    ASSERT_EQ(a[2], 0);
}

TEST(array, write_through_data) {
    std::array<int, 3> a = {1, 2, 3};
    a.data()[1] = 99;
    ASSERT_EQ(a[1], 99);
}

TEST(array, struct_element) {
    struct Point {
        int x, y;
    };
    std::array<Point, 2> pts = {Point{1, 2}, Point{3, 4}};
    ASSERT_EQ(pts[0].x, 1);
    ASSERT_EQ(pts[1].y, 4);
    pts[0].x = 10;
    ASSERT_EQ(pts[0].x, 10);
}

#include "../framework/test.hpp"

TEST(stdlib, itoa_positive) {
    char buf[20];
    itoa(123, buf, 10);
    ASSERT_STR_EQ(buf, "123");
}

TEST(stdlib, itoa_negative) {
    char buf[20];
    itoa(-456, buf, 10);
    ASSERT_STR_EQ(buf, "-456");
}

TEST(stdlib, itoa_hex) {
    char buf[20];
    itoa(255, buf, 16);
    ASSERT_STR_EQ(buf, "ff");
}

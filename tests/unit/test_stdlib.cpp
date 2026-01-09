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

TEST(stdlib, itoa_zero) {
    char buf[20];
    itoa(0, buf, 10);
    ASSERT_STR_EQ(buf, "0");
}

TEST(stdlib, itoa_binary) {
    char buf[20];
    itoa(10, buf, 2);
    ASSERT_STR_EQ(buf, "1010");
    itoa(255, buf, 2);
    ASSERT_STR_EQ(buf, "11111111");
}

TEST(stdlib, itoa_octal) {
    char buf[20];
    itoa(64, buf, 8);
    ASSERT_STR_EQ(buf, "100");
    itoa(255, buf, 8);
    ASSERT_STR_EQ(buf, "377");
}

TEST(stdlib, itoa_large_positive) {
    char buf[20];
    itoa(2147483647, buf, 10);
    ASSERT_STR_EQ(buf, "2147483647");
}

TEST(stdlib, itoa_large_negative) {
    char buf[20];
    itoa(-2147483648, buf, 10);
    ASSERT_STR_EQ(buf, "-2147483648");
}

TEST(stdlib, itoa_single_digit) {
    char buf[20];
    itoa(5, buf, 10);
    ASSERT_STR_EQ(buf, "5");
    itoa(-7, buf, 10);
    ASSERT_STR_EQ(buf, "-7");
}

TEST(stdlib, itoa_hex_uppercase_not_used) {
    char buf[20];
    itoa(255, buf, 16);
    ASSERT_STR_EQ(buf, "ff");
    itoa(10, buf, 16);
    ASSERT_STR_EQ(buf, "a");
}

TEST(stdlib, itoa_base_36) {
    char buf[20];
    itoa(35, buf, 36);
    ASSERT_STR_EQ(buf, "z");
}

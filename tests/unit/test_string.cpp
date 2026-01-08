#include "../framework/test.hpp"

TEST(string, strlen) {
    ASSERT_EQ(strlen("hello"), 5);
    ASSERT_EQ(strlen(""), 0);
    ASSERT_EQ(strlen("a"), 1);
}

TEST(string, strcpy) {
    char dest[20];
    strcpy(dest, "test");
    ASSERT_STR_EQ(dest, "test");
}

TEST(string, strcmp) {
    ASSERT(strcmp("abc", "abc") == 0);
    ASSERT(strcmp("abc", "def") < 0);
    ASSERT(strcmp("def", "abc") > 0);
}

TEST(string, memset) {
    char buf[10];
    memset(buf, 'A', 10);
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(buf[i], 'A');
    }
}

TEST(string, memcpy) {
    char src[] = "hello world";
    char dest[20];
    memcpy(dest, src, strlen(src) + 1);
    ASSERT_STR_EQ(dest, "hello world");
}

TEST(string, strchr) {
    const char *str = "hello world";
    ASSERT(strchr(str, 'w') == str + 6);
    ASSERT(strchr(str, 'z') == nullptr);
    ASSERT(strchr(str, '\0') == str + 11);
}

TEST(string, strnlen) {
    ASSERT_EQ(strnlen("hello", 10), 5);
    ASSERT_EQ(strnlen("hello", 3), 3);
    ASSERT_EQ(strnlen("", 10), 0);
}

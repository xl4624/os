#include "../framework/test.h"

TEST(string, memcmp_equal) {
    char buf1[] = "hello";
    char buf2[] = "hello";
    ASSERT_EQ(memcmp(buf1, buf2, 5), 0);
    ASSERT_EQ(memcmp(buf1, buf2, 3), 0);
}

TEST(string, memcmp_less_than) {
    char buf1[] = "abc";
    char buf2[] = "def";
    ASSERT(memcmp(buf1, buf2, 3) < 0);
    ASSERT(memcmp(buf1, buf2, 1) < 0);
}

TEST(string, memcmp_greater_than) {
    char buf1[] = "xyz";
    char buf2[] = "abc";
    ASSERT(memcmp(buf1, buf2, 3) > 0);
}

TEST(string, memcmp_zero_length) {
    char buf1[] = "hello";
    char buf2[] = "world";
    ASSERT_EQ(memcmp(buf1, buf2, 0), 0);
}

TEST(string, memcmp_partial) {
    char buf1[] = "hello world";
    char buf2[] = "hello there";
    ASSERT_EQ(memcmp(buf1, buf2, 5), 0);
    ASSERT(memcmp(buf1, buf2, 10) != 0);
}

TEST(string, memmove_non_overlapping) {
    char src[] = "hello world";
    char dest[20];
    memmove(dest, src, 12);
    ASSERT_STR_EQ(dest, "hello world");
}

TEST(string, memmove_overlapping_forward) {
    char buf[] = "hello world";
    memmove(buf + 6, buf, 5);
    ASSERT_STR_EQ(buf, "hello hello");
}

TEST(string, memmove_overlapping_backward) {
    char buf[] = "hello world";
    memmove(buf, buf + 6, 5);
    ASSERT_STR_EQ(buf, "world world");
}

TEST(string, memmove_zero_length) {
    char buf[] = "hello";
    memmove(buf, buf, 0);
    ASSERT_STR_EQ(buf, "hello");
}

TEST(string, memmove_returns_dest) {
    char buf[20];
    void *result = memmove(buf, "test", 5);
    ASSERT(result == buf);
}

TEST(string, strcpy_empty) {
    char dest[20];
    strcpy(dest, "");
    ASSERT_STR_EQ(dest, "");
    ASSERT_EQ(dest[0], '\0');
}

TEST(string, strcpy_long_string) {
    char dest[100];
    strcpy(dest, "this is a very long string that tests the strcpy function");
    ASSERT_STR_EQ(dest,
                  "this is a very long string that tests the strcpy function");
}

TEST(string, strcpy_returns_dest) {
    char dest[20];
    char *result = strcpy(dest, "test");
    ASSERT(result == dest);
}

TEST(string, memcpy_non_overlapping) {
    char src[] = "hello world";
    char dest[20];
    memcpy(dest, src, 12);
    ASSERT_STR_EQ(dest, "hello world");
}

TEST(string, memcpy_zero_length) {
    char buf[] = "hello";
    memcpy(buf, "xxxxx", 0);
    ASSERT_STR_EQ(buf, "hello");
}

TEST(string, memcpy_returns_dest) {
    char buf[20];
    void *result = memcpy(buf, "test", 5);
    ASSERT(result == buf);
}

TEST(string, strcmp_empty) {
    ASSERT_EQ(strcmp("", ""), 0);
    ASSERT(strcmp("a", "") > 0);
    ASSERT(strcmp("", "a") < 0);
}

TEST(string, strcmp_equal) {
    ASSERT_EQ(strcmp("hello", "hello"), 0);
    ASSERT_EQ(strcmp("", ""), 0);
}

TEST(string, strcmp_different_lengths) {
    ASSERT(strcmp("hello", "hello world") < 0);
    ASSERT(strcmp("hello world", "hello") > 0);
}

TEST(string, memset_entire_buffer) {
    char buf[10];
    memset(buf, 'A', 10);
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(buf[i], 'A');
    }
}

TEST(string, memset_partial) {
    char buf[] = "hello world";
    memset(buf + 6, 'X', 3);
    ASSERT_STR_EQ(buf, "hello XXXld");
}

TEST(string, memset_zero) {
    char buf[] = "hello";
    memset(buf, 0, 5);
    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(buf[i], '\0');
    }
}

TEST(string, memset_returns_dest) {
    char buf[20];
    void *result = memset(buf, 'X', 5);
    ASSERT(result == buf);
}

TEST(string, strlen_empty) {
    ASSERT_EQ(strlen(""), 0);
}

TEST(string, strlen_single_char) {
    ASSERT_EQ(strlen("a"), 1);
}

TEST(string, strlen_with_spaces) {
    ASSERT_EQ(strlen("hello world"), 11);
}

TEST(string, strchr_first_char) {
    const char *str = "hello";
    ASSERT(strchr(str, 'h') == str);
}

TEST(string, strchr_last_char) {
    const char *str = "hello";
    ASSERT(strchr(str, 'o') == str + 4);
}

TEST(string, strchr_not_found) {
    const char *str = "hello";
    ASSERT(strchr(str, 'z') == nullptr);
}

TEST(string, strchr_null_terminator) {
    const char *str = "hello";
    ASSERT(strchr(str, '\0') == str + 5);
}

TEST(string, strnlen_larger_than_string) {
    ASSERT_EQ(strnlen("hi", 100), 2);
}

TEST(string, strnlen_exact_length) {
    ASSERT_EQ(strnlen("hi", 2), 2);
}

TEST(string, strnlen_zero_max) {
    ASSERT_EQ(strnlen("hello", 0), 0);
}

TEST(string, memchr_found) {
    char buf[] = "hello world";
    ASSERT(memchr(buf, 'w', 11) == buf + 6);
    ASSERT(memchr(buf, 'h', 11) == buf);
    ASSERT(memchr(buf, 'd', 11) == buf + 10);
}

TEST(string, memchr_not_found) {
    char buf[] = "hello";
    ASSERT(memchr(buf, 'z', 5) == nullptr);
    ASSERT(memchr(buf, 'w', 3) == nullptr);
}

TEST(string, memchr_zero_length) {
    char buf[] = "hello";
    ASSERT(memchr(buf, 'h', 0) == nullptr);
}

TEST(string, memchr_null_byte) {
    char buf[] = "hello\0world";
    ASSERT(memchr(buf, '\0', 11) == buf + 5);
}

TEST(string, strcat_basic) {
    char dest[20] = "Hello";
    strcat(dest, " World");
    ASSERT_STR_EQ(dest, "Hello World");
}

TEST(string, strcat_empty_src) {
    char dest[20] = "Hello";
    strcat(dest, "");
    ASSERT_STR_EQ(dest, "Hello");
}

TEST(string, strcat_empty_dest) {
    char dest[20] = "";
    strcat(dest, "Hello");
    ASSERT_STR_EQ(dest, "Hello");
}

TEST(string, strcat_returns_dest) {
    char dest[20] = "Hello";
    char *result = strcat(dest, " World");
    ASSERT(result == dest);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
TEST(string, strncat_basic) {
    char dest[20] = "Hello";
    strncat(dest, " World", 6);
    ASSERT_STR_EQ(dest, "Hello World");
}
#pragma GCC diagnostic pop

TEST(string, strncat_limit) {
    char dest[20] = "Hello";
    strncat(dest, " World", 3);
    ASSERT_STR_EQ(dest, "Hello Wo");
}

TEST(string, strncat_zero_n) {
    char dest[20] = "Hello";
    strncat(dest, " World", 0);
    ASSERT_STR_EQ(dest, "Hello");
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
TEST(string, strncat_returns_dest) {
    char dest[20] = "Hello";
    char *result = strncat(dest, " World", 6);
    ASSERT(result == dest);
}
#pragma GCC diagnostic pop

TEST(string, strncmp_equal) {
    ASSERT_EQ(strncmp("hello", "hello", 5), 0);
    ASSERT_EQ(strncmp("hello", "hello", 3), 0);
    ASSERT_EQ(strncmp("hello", "hello", 10), 0);
}

TEST(string, strncmp_less_than) {
    ASSERT(strncmp("abc", "def", 3) < 0);
    ASSERT(strncmp("abc", "abd", 3) < 0);
}

TEST(string, strncmp_greater_than) {
    ASSERT(strncmp("def", "abc", 3) > 0);
    ASSERT(strncmp("abd", "abc", 3) > 0);
}

TEST(string, strncmp_partial) {
    ASSERT_EQ(strncmp("hello", "help", 3), 0);
    ASSERT(strncmp("hello", "help", 4) < 0);
}

TEST(string, strncmp_zero_length) {
    ASSERT_EQ(strncmp("abc", "def", 0), 0);
}

TEST(string, strncpy_basic) {
    char dest[20];
    strncpy(dest, "hello", 6);
    ASSERT_STR_EQ(dest, "hello");
}

TEST(string, strncpy_pad_with_nulls) {
    char dest[10] = "XXXXXXXXX";
    strncpy(dest, "hi", 5);
    ASSERT_EQ(dest[0], 'h');
    ASSERT_EQ(dest[1], 'i');
    ASSERT_EQ(dest[2], '\0');
    ASSERT_EQ(dest[3], '\0');
    ASSERT_EQ(dest[4], '\0');
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
TEST(string, strncpy_truncates) {
    char dest[20];
    strncpy(dest, "hello world", 5);
    ASSERT_EQ(dest[0], 'h');
    ASSERT_EQ(dest[4], 'o');
    dest[5] = '\0';
    ASSERT_STR_EQ(dest, "hello");
}
#pragma GCC diagnostic pop

TEST(string, strncpy_empty_src) {
    char dest[20] = "original";
    strncpy(dest, "", 5);
    ASSERT_EQ(dest[0], '\0');
    ASSERT_EQ(dest[1], '\0');
    ASSERT_EQ(dest[2], '\0');
    ASSERT_EQ(dest[3], '\0');
    ASSERT_EQ(dest[4], '\0');
}

TEST(string, strncpy_returns_dest) {
    char dest[20];
    char *result = strncpy(dest, "test", 5);
    ASSERT(result == dest);
}

TEST(string, strstr_basic) {
    ASSERT(strstr("hello world", "world") != nullptr);
    ASSERT(strstr("hello world", "hello") != nullptr);
}

TEST(string, strstr_not_found) {
    ASSERT(strstr("hello world", "foo") == nullptr);
    ASSERT(strstr("hello", "hello world") == nullptr);
}

TEST(string, strstr_empty_needle) {
    char *result = strstr("hello", "");
    ASSERT(result != nullptr);
    ASSERT_STR_EQ(result, "hello");
}

TEST(string, strstr_multiple_occurrences) {
    char *result = strstr("hello hello world", "hello");
    ASSERT(result != nullptr);
    ASSERT_EQ(result[0], 'h');
}

TEST(string, strstr_returns_pointer) {
    char buf[] = "hello world";
    ASSERT(strstr(buf, "world") == buf + 6);
}

TEST(string, strtok_basic) {
    char buf[] = "hello,world,test";
    char *token = strtok(buf, ",");
    ASSERT_STR_EQ(token, "hello");
    token = strtok(nullptr, ",");
    ASSERT_STR_EQ(token, "world");
    token = strtok(nullptr, ",");
    ASSERT_STR_EQ(token, "test");
    token = strtok(nullptr, ",");
    ASSERT(token == nullptr);
}

TEST(string, strtok_multiple_delims) {
    char buf[] = "hello world\ttest";
    char *token = strtok(buf, " \t");
    ASSERT_STR_EQ(token, "hello");
    token = strtok(nullptr, " \t");
    ASSERT_STR_EQ(token, "world");
    token = strtok(nullptr, " \t");
    ASSERT_STR_EQ(token, "test");
}

TEST(string, strtok_empty_string) {
    char buf[] = "";
    char *token = strtok(buf, ",");
    ASSERT(token == nullptr);
}

TEST(string, strtok_no_delim_found) {
    char buf[] = "hello";
    char *token = strtok(buf, ",");
    ASSERT_STR_EQ(token, "hello");
    token = strtok(nullptr, ",");
    ASSERT(token == nullptr);
}

TEST(string, strtok_consecutive_delims) {
    char buf[] = "hello,,world";
    char *token = strtok(buf, ",");
    ASSERT_STR_EQ(token, "hello");
    token = strtok(nullptr, ",");
    ASSERT_STR_EQ(token, "world");
}

TEST(string, strtok_r_basic) {
    char buf[] = "hello,world,test";
    char *saveptr;
    char *token = strtok_r(buf, ",", &saveptr);
    ASSERT_STR_EQ(token, "hello");
    token = strtok_r(nullptr, ",", &saveptr);
    ASSERT_STR_EQ(token, "world");
    token = strtok_r(nullptr, ",", &saveptr);
    ASSERT_STR_EQ(token, "test");
    token = strtok_r(nullptr, ",", &saveptr);
    ASSERT(token == nullptr);
}

TEST(string, strtok_r_multiple_strings) {
    char buf1[] = "a,b,c";
    char buf2[] = "x,y,z";
    char *saveptr1, *saveptr2;

    char *t1 = strtok_r(buf1, ",", &saveptr1);
    char *t2 = strtok_r(buf2, ",", &saveptr2);

    ASSERT_STR_EQ(t1, "a");
    ASSERT_STR_EQ(t2, "x");

    t1 = strtok_r(nullptr, ",", &saveptr1);
    t2 = strtok_r(nullptr, ",", &saveptr2);

    ASSERT_STR_EQ(t1, "b");
    ASSERT_STR_EQ(t2, "y");
}

TEST(string, strspn_basic) {
    ASSERT_EQ(strspn("hello", "helo"), 5);
    ASSERT_EQ(strspn("hello world", "helo"), 5);
    ASSERT_EQ(strspn("   hello", " "), 3);
}

TEST(string, strspn_no_match) {
    ASSERT_EQ(strspn("hello", "xyz"), 0);
    ASSERT_EQ(strspn("", "abc"), 0);
}

TEST(string, strspn_all_match) {
    ASSERT_EQ(strspn("aaa", "a"), 3);
}

TEST(string, strcspn_basic) {
    ASSERT_EQ(strcspn("hello,world", ","), 5);
    ASSERT_EQ(strcspn("hello world", " "), 5);
    ASSERT_EQ(strcspn("hello", "xyz"), 5);
}

TEST(string, strcspn_no_match) {
    ASSERT_EQ(strcspn("hello", "xyz"), 5);
}

TEST(string, strcspn_immediate_match) {
    ASSERT_EQ(strcspn(",hello", ","), 0);
}

TEST(string, strcspn_empty_string) {
    ASSERT_EQ(strcspn("", "abc"), 0);
}

// strchr must return the FIRST occurrence, not any later one.
TEST(string, strchr_repeated_char_returns_first) {
    const char *str = "aabba";
    ASSERT(strchr(str, 'a') == str);      // first 'a' at index 0
    ASSERT(strchr(str, 'b') == str + 2);  // first 'b' at index 2
}

// strcmp is case-sensitive: 'a' (97) > 'A' (65) in ASCII.
TEST(string, strcmp_case_sensitive) {
    ASSERT_NE(strcmp("abc", "ABC"), 0);
    ASSERT(strcmp("abc", "ABC") > 0);
    ASSERT(strcmp("A", "a") < 0);
}

// memcmp uses unsigned byte comparison; bytes with values > 127 must order
// as larger than bytes with values < 128.
TEST(string, memcmp_unsigned_byte_comparison) {
    const unsigned char big[] = {0xFF};
    const unsigned char small[] = {0x01};
    ASSERT(memcmp(big, small, 1) > 0);
    ASSERT(memcmp(small, big, 1) < 0);
    // 0x80 (128) must compare greater than 0x7F (127).
    const unsigned char a[] = {0x80};
    const unsigned char b[] = {0x7F};
    ASSERT(memcmp(a, b, 1) > 0);
}

// strtok must skip leading delimiters.
TEST(string, strtok_leading_delimiters) {
    char buf[] = ",,,hello,world";
    char *token = strtok(buf, ",");
    ASSERT(token != nullptr);
    ASSERT_STR_EQ(token, "hello");
    token = strtok(nullptr, ",");
    ASSERT_STR_EQ(token, "world");
    token = strtok(nullptr, ",");
    ASSERT(token == nullptr);
}

// strstr with an empty haystack and a non-empty needle returns null.
TEST(string, strstr_empty_haystack) {
    ASSERT(strstr("", "a") == nullptr);
    ASSERT(strstr("", "abc") == nullptr);
}

// strspn with an empty accept set must return 0 for any string.
TEST(string, strspn_empty_accept_set) {
    ASSERT_EQ(strspn("hello", ""), 0u);
    ASSERT_EQ(strspn("", ""), 0u);
}

// strcspn with an empty reject set: no character is rejected, so the entire
// string qualifies and the return value equals strlen(s).
TEST(string, strcspn_empty_reject_set) {
    ASSERT_EQ(strcspn("hello", ""), 5u);
    ASSERT_EQ(strcspn("", ""), 0u);
}

// memchr must return a pointer to the FIRST occurrence.
TEST(string, memchr_multiple_occurrences_returns_first) {
    char buf[] = "abcabc";
    ASSERT(memchr(buf, 'a', 6) == buf);
    ASSERT(memchr(buf, 'b', 6) == buf + 1);
}

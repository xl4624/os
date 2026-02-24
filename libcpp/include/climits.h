#pragma once

#include <limits.h>

namespace std {

constexpr int char_bit = CHAR_BIT;
constexpr int schar_min = SCHAR_MIN;
constexpr int schar_max = SCHAR_MAX;
constexpr unsigned int uchar_max = UCHAR_MAX;
constexpr int char_min = CHAR_MIN;
constexpr int char_max = CHAR_MAX;

constexpr int shrt_min = SHRT_MIN;
constexpr int shrt_max = SHRT_MAX;
constexpr unsigned int ushrt_max = USHRT_MAX;

constexpr int int_min = INT_MIN;
constexpr int int_max = INT_MAX;
constexpr unsigned int uint_max = UINT_MAX;

constexpr long long int llong_min = LLONG_MIN;
constexpr long long int llong_max = LLONG_MAX;
constexpr unsigned long long int ullong_max = ULLONG_MAX;

}  // namespace std

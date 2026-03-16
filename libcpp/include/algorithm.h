#pragma once

namespace std {

template <typename T>
constexpr const T& min(const T& a, const T& b) {
  return (a < b) ? a : b;
}

template <typename T>
constexpr const T& max(const T& a, const T& b) {
  return (a > b) ? a : b;
}

template <typename T>
constexpr T& min(T& a, T& b) {
  return (a < b) ? a : b;
}

template <typename T>
constexpr T& max(T& a, T& b) {
  return (a > b) ? a : b;
}

// Delete rvalue overloads because returning a const ref to a temporary would dangle.
template <typename T>
const T& min(const T&&, const T&&) = delete;

template <typename T>
const T& max(const T&&, const T&&) = delete;

template <typename InputIt1, typename InputIt2>
constexpr bool equal(InputIt1 first1, InputIt1 last1, InputIt2 first2) {
  for (; first1 != last1; ++first1, ++first2) {
    if (!(*first1 == *first2)) {
      return false;
    }
  }
  return true;
}

template <typename InputIt1, typename InputIt2>
constexpr bool lexicographical_compare(InputIt1 first1, InputIt1 last1, InputIt2 first2,
                                       InputIt2 last2) {
  for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
    if (*first1 < *first2) {
      return true;
    }
    if (*first2 < *first1) {
      return false;
    }
  }
  return first1 == last1 && first2 != last2;
}

}  // namespace std

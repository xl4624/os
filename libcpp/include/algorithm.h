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

}  // namespace std

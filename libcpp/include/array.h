#pragma once

#include <algorithm.h>
#include <cstddef.h>
#include <iterator.h>

namespace std {

// _data uses max(N,1) so array<T,0> has valid (but uncallable) accessors
// without a separate partial specialisation
template <typename T, size_t N>
struct array {
  T _data[N > 0 ? N : 1];

  using value_type = T;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using reference = value_type&;
  using const_reference = const value_type&;
  using iterator = value_type*;
  using const_iterator = const value_type*;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr reference operator[](size_type pos) { return _data[pos]; }
  constexpr const_reference operator[](size_type pos) const { return _data[pos]; }
  constexpr reference at(size_type pos) { return _data[pos]; }
  constexpr const_reference at(size_type pos) const { return _data[pos]; }
  constexpr reference front() { return _data[0]; }
  constexpr const_reference front() const { return _data[0]; }
  constexpr reference back() { return _data[N - 1]; }
  constexpr const_reference back() const { return _data[N - 1]; }
  constexpr pointer data() noexcept { return N > 0 ? _data : nullptr; }
  constexpr const_pointer data() const noexcept { return N > 0 ? _data : nullptr; }

  constexpr iterator begin() noexcept { return N > 0 ? _data : nullptr; }
  constexpr const_iterator begin() const noexcept { return N > 0 ? _data : nullptr; }
  constexpr const_iterator cbegin() const noexcept { return N > 0 ? _data : nullptr; }
  constexpr iterator end() noexcept { return N > 0 ? _data + N : nullptr; }
  constexpr const_iterator end() const noexcept { return N > 0 ? _data + N : nullptr; }
  constexpr const_iterator cend() const noexcept { return N > 0 ? _data + N : nullptr; }

  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(begin());
  }

  [[nodiscard]] constexpr bool empty() const noexcept { return N == 0; }
  [[nodiscard]] constexpr size_type size() const noexcept { return N; }
  [[nodiscard]] constexpr size_type max_size() const noexcept { return N; }

  constexpr void fill(const_reference value) {
    for (size_type i = 0; i < N; ++i) {
      _data[i] = value;
    }
  }

  constexpr void swap(array& other) noexcept {
    for (size_type i = 0; i < N; ++i) {
      value_type tmp = _data[i];
      _data[i] = other._data[i];
      other._data[i] = tmp;
    }
  }
};

// Array comparison operators.

template <typename T, size_t N>
inline bool operator==(const array<T, N>& a, const array<T, N>& b) {
  return std::equal(a.begin(), a.end(), b.begin());
}

template <typename T, size_t N>
inline bool operator!=(const array<T, N>& a, const array<T, N>& b) {
  return !(a == b);
}

template <typename T, size_t N>
inline bool operator<(const array<T, N>& a, const array<T, N>& b) {
  return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

template <typename T, size_t N>
inline bool operator>(const array<T, N>& a, const array<T, N>& b) {
  return b < a;
}

template <typename T, size_t N>
inline bool operator<=(const array<T, N>& a, const array<T, N>& b) {
  return !(a > b);
}

template <typename T, size_t N>
inline bool operator>=(const array<T, N>& a, const array<T, N>& b) {
  return !(a < b);
}

}  // namespace std

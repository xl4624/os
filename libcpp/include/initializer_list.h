#pragma once

#include <stddef.h>

namespace std {

template <typename T>
class initializer_list {
 public:
  using value_type = T;
  using reference = const T&;
  using const_reference = const T&;
  using size_type = size_t;
  using iterator = const T*;

  constexpr initializer_list() noexcept : _begin(nullptr), _size(0) {}
  constexpr initializer_list(const T* first, size_type count) noexcept
      : _begin(first), _size(count) {}

  constexpr const T* begin() const noexcept { return _begin; }
  constexpr const T* end() const noexcept { return _begin + _size; }
  constexpr size_type size() const noexcept { return _size; }

 private:
  const T* _begin;
  size_type _size;
};

}  // namespace std

#pragma once

#include <array.h>
#include <cstddef.h>
#include <type_traits.h>

namespace std {

inline constexpr size_t dynamic_extent = static_cast<size_t>(-1);

// span<T>: a non-owning view over a contiguous sequence of T objects.
// The extent is always dynamic (stored at runtime).
template <typename T, size_t Extent = dynamic_extent>
class span {
 public:
  using element_type = T;
  using value_type = remove_cv_t<T>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using iterator = T*;
  using const_iterator = const T*;

  static constexpr size_t extent = Extent;

  // Constructors

  constexpr span() noexcept : data_(nullptr), size_(0) {}

  constexpr span(T* ptr, size_type count) noexcept : data_(ptr), size_(count) {}

  constexpr span(T* first, T* last) noexcept
      : data_(first), size_(static_cast<size_type>(last - first)) {}

  template <size_t N>
  constexpr span(T (&arr)[N]) noexcept : data_(arr), size_(N) {}

  template <size_t N>
  constexpr span(array<T, N>& arr) noexcept : data_(arr.data()), size_(N) {}

  template <size_t N>
  constexpr span(const array<value_type, N>& arr) noexcept : data_(arr.data()), size_(N) {}

  constexpr span(const span&) noexcept = default;
  constexpr span& operator=(const span&) noexcept = default;

  // Element access

  [[nodiscard]] constexpr reference operator[](size_type idx) const noexcept {
    return data_[idx];
  }

  [[nodiscard]] constexpr reference front() const noexcept { return data_[0]; }
  [[nodiscard]] constexpr reference back() const noexcept { return data_[size_ - 1]; }
  [[nodiscard]] constexpr pointer data() const noexcept { return data_; }

  // Iterators

  [[nodiscard]] constexpr iterator begin() const noexcept { return data_; }
  [[nodiscard]] constexpr iterator end() const noexcept { return data_ + size_; }
  [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return data_; }
  [[nodiscard]] constexpr const_iterator cend() const noexcept { return data_ + size_; }

  // Observers

  [[nodiscard]] constexpr size_type size() const noexcept { return size_; }
  [[nodiscard]] constexpr size_type size_bytes() const noexcept { return size_ * sizeof(T); }
  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

  // Subspans

  [[nodiscard]] constexpr span<T> first(size_type count) const noexcept {
    return {data_, count};
  }

  [[nodiscard]] constexpr span<T> last(size_type count) const noexcept {
    return {data_ + size_ - count, count};
  }

  [[nodiscard]] constexpr span<T> subspan(size_type offset,
                                          size_type count = dynamic_extent) const noexcept {
    size_type actual = (count == dynamic_extent) ? size_ - offset : count;
    return {data_ + offset, actual};
  }

 private:
  T* data_;
  size_type size_;
};

// Deduction guides.

template <typename T, size_t N>
span(T (&)[N]) -> span<T, N>;

template <typename T, size_t N>
span(array<T, N>&) -> span<T, N>;

template <typename T, size_t N>
span(const array<T, N>&) -> span<const T, N>;

}  // namespace std

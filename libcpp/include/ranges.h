#pragma once

#include <iterator.h>

namespace std {

namespace ranges {

// Wraps a range and exposes rbegin/rend as begin/end so a range-based for
// loop iterates in reverse order.
template <typename R>
class reverse_view {
 public:
  constexpr explicit reverse_view(R& r) : range_(r) {}

  constexpr auto begin() const { return std::rbegin(range_); }
  constexpr auto end() const { return std::rend(range_); }

 private:
  R& range_;
};

}  // namespace ranges

namespace views {

struct reverse_fn {
  template <typename R>
  [[nodiscard]] constexpr ranges::reverse_view<R> operator()(R& r) const {
    return ranges::reverse_view<R>(r);
  }

  template <typename R>
  [[nodiscard]] constexpr ranges::reverse_view<const R> operator()(const R& r) const {
    return ranges::reverse_view<const R>(r);
  }
};

inline constexpr reverse_fn reverse{};

}  // namespace views

}  // namespace std

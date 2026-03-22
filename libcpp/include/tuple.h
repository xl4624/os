#pragma once

#include <cstddef.h>
#include <type_traits.h>
#include <utility.h>

namespace std {

// Index-tagged leaf that stores one element.
template <size_t I, typename T>
struct TupleLeaf {
  T value;

  constexpr TupleLeaf() : value() {}
  explicit constexpr TupleLeaf(const T& v) : value(v) {}

  template <typename U>
  explicit constexpr TupleLeaf(U&& v) : value(std::forward<U>(v)) {}
};

// Recursive tuple implementation via multiple inheritance.
template <typename IndexSeq, typename... Ts>
struct TupleImpl;

// Empty specialization to avoid ambiguous constructors.
template <>
struct TupleImpl<index_sequence<>> {
  constexpr TupleImpl() = default;
};

template <size_t... Is, typename... Ts>
struct TupleImpl<index_sequence<Is...>, Ts...> : TupleLeaf<Is, Ts>... {
  constexpr TupleImpl() = default;

  explicit constexpr TupleImpl(const Ts&... args) : TupleLeaf<Is, Ts>(args)... {}

  template <typename... Us, enable_if_t<sizeof...(Us) == sizeof...(Ts), int> = 0>
  explicit constexpr TupleImpl(Us&&... args) : TupleLeaf<Is, Ts>(std::forward<Us>(args))... {}
};

template <typename... Ts>
class tuple : public TupleImpl<make_index_sequence<sizeof...(Ts)>, Ts...> {
  using Base = TupleImpl<make_index_sequence<sizeof...(Ts)>, Ts...>;

 public:
  constexpr tuple() = default;

  explicit constexpr tuple(const Ts&... args) : Base(args...) {}

  template <typename... Us, enable_if_t<sizeof...(Us) == sizeof...(Ts), int> = 0>
  explicit constexpr tuple(Us&&... args) : Base(std::forward<Us>(args)...) {}

  tuple(const tuple&) = default;
  tuple(tuple&&) = default;
  tuple& operator=(const tuple&) = default;
  tuple& operator=(tuple&&) = default;
};

// Specialization for empty tuple.
template <>
class tuple<> {
 public:
  constexpr tuple() = default;
};

// Deduction guide.
template <typename... Ts>
tuple(Ts...) -> tuple<Ts...>;

// tuple_size
template <typename T>
struct tuple_size;

template <typename... Ts>
struct tuple_size<tuple<Ts...>> : integral_constant<size_t, sizeof...(Ts)> {};

template <typename T>
inline constexpr size_t tuple_size_v = tuple_size<T>::value;

// tuple_element
template <size_t I, typename T>
struct tuple_element;

template <size_t I, typename Head, typename... Tail>
struct tuple_element<I, tuple<Head, Tail...>> : tuple_element<I - 1, tuple<Tail...>> {};

template <typename Head, typename... Tail>
struct tuple_element<0, tuple<Head, Tail...>> {
  using type = Head;
};

template <size_t I, typename T>
using tuple_element_t = typename tuple_element<I, T>::type;

// std::get by index
template <size_t I, typename T>
constexpr T& get_leaf(TupleLeaf<I, T>& leaf) noexcept {
  return leaf.value;
}

template <size_t I, typename T>
constexpr const T& get_leaf(const TupleLeaf<I, T>& leaf) noexcept {
  return leaf.value;
}

template <size_t I, typename T>
constexpr T&& get_leaf(TupleLeaf<I, T>&& leaf) noexcept {
  return std::move(leaf.value);
}

template <size_t I, typename... Ts>
constexpr auto& get(tuple<Ts...>& t) noexcept {
  return get_leaf<I>(t);
}

template <size_t I, typename... Ts>
constexpr const auto& get(const tuple<Ts...>& t) noexcept {
  return get_leaf<I>(t);
}

template <size_t I, typename... Ts>
constexpr auto&& get(tuple<Ts...>&& t) noexcept {
  return get_leaf<I>(std::move(t));
}

// make_tuple
template <typename... Ts>
constexpr tuple<decay_t<Ts>...> make_tuple(Ts&&... args) {
  return tuple<decay_t<Ts>...>(std::forward<Ts>(args)...);
}

// tie
template <typename... Ts>
constexpr tuple<Ts&...> tie(Ts&... args) noexcept {
  return tuple<Ts&...>(args...);
}

// Comparison operators.
namespace detail {

template <size_t I, size_t N>
struct TupleCompare {
  template <typename T, typename U>
  static constexpr bool equal(const T& a, const U& b) {
    return get<I>(a) == get<I>(b) && TupleCompare<I + 1, N>::equal(a, b);
  }

  template <typename T, typename U>
  static constexpr bool less(const T& a, const U& b) {
    if (get<I>(a) < get<I>(b)) return true;
    if (get<I>(b) < get<I>(a)) return false;
    return TupleCompare<I + 1, N>::less(a, b);
  }
};

template <size_t N>
struct TupleCompare<N, N> {
  template <typename T, typename U>
  static constexpr bool equal(const T&, const U&) {
    return true;
  }

  template <typename T, typename U>
  static constexpr bool less(const T&, const U&) {
    return false;
  }
};

}  // namespace detail

template <typename... Ts, typename... Us>
constexpr bool operator==(const tuple<Ts...>& a, const tuple<Us...>& b) {
  static_assert(sizeof...(Ts) == sizeof...(Us), "Tuple sizes must match");
  return detail::TupleCompare<0, sizeof...(Ts)>::equal(a, b);
}

template <typename... Ts, typename... Us>
constexpr bool operator!=(const tuple<Ts...>& a, const tuple<Us...>& b) {
  return !(a == b);
}

template <typename... Ts, typename... Us>
constexpr bool operator<(const tuple<Ts...>& a, const tuple<Us...>& b) {
  static_assert(sizeof...(Ts) == sizeof...(Us), "Tuple sizes must match");
  return detail::TupleCompare<0, sizeof...(Ts)>::less(a, b);
}

}  // namespace std

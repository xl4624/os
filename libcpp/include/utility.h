#pragma once

#include <cstddef.h>
#include <type_traits.h>

namespace std {

template <typename T>
constexpr T&& forward(typename std::remove_reference<T>::type& t) noexcept {
  return static_cast<T&&>(t);
}

template <typename T>
constexpr T&& forward(typename std::remove_reference<T>::type&& t) noexcept {
  static_assert(!std::is_lvalue_reference<T>::value, "Cannot forward an rvalue as an lvalue");
  return static_cast<T&&>(t);
}

template <typename T>
constexpr typename std::remove_reference<T>::type&& move(T&& t) noexcept {
  return static_cast<typename std::remove_reference<T>::type&&>(t);
}

template <typename T>
constexpr void swap(T& a, T& b) noexcept {
  T tmp = std::move(a);
  a = std::move(b);
  b = std::move(tmp);
}

template <typename T, size_t N>
constexpr void swap(T (&a)[N], T (&b)[N]) noexcept {
  for (size_t i = 0; i < N; ++i) {
    swap(a[i], b[i]);
  }
}

template <typename T1, typename T2>
struct pair {
  using first_type = T1;
  using second_type = T2;

  T1 first;
  T2 second;

  constexpr pair() : first(), second() {}
  constexpr pair(const T1& a, const T2& b) : first(a), second(b) {}

  template <typename U1, typename U2>
  constexpr pair(U1&& a, U2&& b) : first(std::forward<U1>(a)), second(std::forward<U2>(b)) {}

  template <typename U1, typename U2>
  constexpr pair(const pair<U1, U2>& other) : first(other.first), second(other.second) {}

  template <typename U1, typename U2>
  constexpr pair(pair<U1, U2>&& other)
      : first(std::forward<U1>(other.first)), second(std::forward<U2>(other.second)) {}

  pair(const pair&) = default;
  pair(pair&&) = default;
  pair& operator=(const pair&) = default;
  pair& operator=(pair&&) = default;

  constexpr void swap(pair& other) noexcept {
    std::swap(first, other.first);
    std::swap(second, other.second);
  }
};

template <typename T1, typename T2>
constexpr bool operator==(const pair<T1, T2>& a, const pair<T1, T2>& b) {
  return a.first == b.first && a.second == b.second;
}

template <typename T1, typename T2>
constexpr bool operator!=(const pair<T1, T2>& a, const pair<T1, T2>& b) {
  return !(a == b);
}

template <typename T1, typename T2>
constexpr bool operator<(const pair<T1, T2>& a, const pair<T1, T2>& b) {
  if (a.first < b.first) return true;
  if (b.first < a.first) return false;
  return a.second < b.second;
}

template <typename T1, typename T2>
constexpr bool operator>(const pair<T1, T2>& a, const pair<T1, T2>& b) {
  return b < a;
}

template <typename T1, typename T2>
constexpr bool operator<=(const pair<T1, T2>& a, const pair<T1, T2>& b) {
  return !(b < a);
}

template <typename T1, typename T2>
constexpr bool operator>=(const pair<T1, T2>& a, const pair<T1, T2>& b) {
  return !(a < b);
}

template <typename T1, typename T2>
constexpr pair<decay_t<T1>, decay_t<T2>> make_pair(T1&& a, T2&& b) {
  return pair<decay_t<T1>, decay_t<T2>>(std::forward<T1>(a), std::forward<T2>(b));
}

// Deduction guide: pair{1, 2.0} -> pair<int, double>
template <typename T1, typename T2>
pair(T1, T2) -> pair<T1, T2>;

template <typename...>
using void_t = void;

template <typename T>
struct is_nothrow_default_constructible : integral_constant<bool, noexcept(T())> {};

template <typename T>
inline constexpr bool is_nothrow_default_constructible_v =
    is_nothrow_default_constructible<T>::value;

template <typename T>
struct is_nothrow_copy_constructible
    : integral_constant<bool, noexcept(T(std::declval<const T&>()))> {};

template <typename T>
inline constexpr bool is_nothrow_copy_constructible_v = is_nothrow_copy_constructible<T>::value;

template <typename T>
struct is_nothrow_move_constructible : integral_constant<bool, noexcept(T(std::declval<T&&>()))> {};

template <typename T>
inline constexpr bool is_nothrow_move_constructible_v = is_nothrow_move_constructible<T>::value;

template <typename T>
struct is_copy_assignable
    : integral_constant<bool, noexcept(std::declval<T&>() = std::declval<const T&>())> {};

template <typename T>
inline constexpr bool is_copy_assignable_v = is_copy_assignable_v<T>;

template <typename T>
struct is_move_assignable
    : integral_constant<bool, noexcept(std::declval<T&>() = std::declval<T&&>())> {};

template <typename T>
inline constexpr bool is_move_assignable_v = is_move_assignable_v<T>;

template <class T>
struct is_trivially_destructible : integral_constant<bool, __has_trivial_destructor(T)> {};

template <class T>
inline constexpr bool is_trivially_destructible_v = is_trivially_destructible<T>::value;

template <class T>
struct is_trivially_copyable : integral_constant<bool, __is_trivially_copyable(T)> {};

template <class T>
inline constexpr bool is_trivially_copyable_v = is_trivially_copyable<T>::value;

template <class T>
struct is_trivially_constructible : integral_constant<bool, __is_trivially_constructible(T)> {};

template <class T>
inline constexpr bool is_trivially_constructible_v = is_trivially_constructible<T>::value;

template <class T>
struct is_trivially_assignable : integral_constant<bool, __is_trivially_assignable(T, T)> {};

template <class T>
inline constexpr bool is_trivially_assignable_v = is_trivially_assignable<T>::value;

template <typename T>
struct is_nothrow_destructible : integral_constant<bool, noexcept(~declval<T>())> {};

template <typename T>
inline constexpr bool is_nothrow_destructible_v = is_nothrow_destructible<T>::value;

template <class T, class U>
struct is_assignable : integral_constant<bool, noexcept(std::declval<T>() = std::declval<U>())> {};

template <class T, class U>
inline constexpr bool is_assignable_v = is_assignable<T, U>::value;

template <class T>
struct is_destructible : integral_constant<bool, noexcept(~declval<T>())> {};

template <class T>
inline constexpr bool is_destructible_v = is_destructible<T>::value;

template <class T, class... Args>
struct is_constructible : integral_constant<bool, noexcept(T(declval<Args>()...))> {};

template <class T, class... Args>
inline constexpr bool is_constructible_v = is_constructible<T, Args...>::value;

template <class T>
struct is_default_constructible : is_constructible<T> {};

template <class T>
inline constexpr bool is_default_constructible_v = is_default_constructible<T>::value;

template <class T>
struct is_copy_constructible : is_constructible<T, const T&> {};

template <class T>
inline constexpr bool is_copy_constructible_v = is_copy_constructible<T>::value;

template <class T>
struct is_move_constructible : is_constructible<T, T&&> {};

template <class T>
inline constexpr bool is_move_constructible_v = is_move_constructible<T>::value;

template <class T>
struct is_pod : integral_constant<bool, is_trivially_destructible<T>::value &&
                                            is_trivially_copyable<T>::value &&
                                            is_trivially_constructible<T>::value> {};

template <class T>
inline constexpr bool is_pod_v = is_pod<T>::value;

template <class T>
struct is_literal_type : integral_constant<bool, __is_literal_type(T)> {};

template <class T>
inline constexpr bool is_literal_type_v = is_literal_type<T>::value;

template <class T>
struct is_standard_layout : integral_constant<bool, __is_standard_layout(T)> {};

template <class T>
inline constexpr bool is_standard_layout_v = is_standard_layout<T>::value;

template <class T>
struct is_trivial : integral_constant<bool, __is_trivial(T)> {};

template <class T>
inline constexpr bool is_trivial_v = is_trivial<T>::value;

template <class T>
struct is_signed : integral_constant<bool, (static_cast<T>(-1) < static_cast<T>(0))> {};

template <class T>
inline constexpr bool is_signed_v = is_signed<T>::value;

template <class T>
struct is_unsigned : integral_constant<bool, !is_signed<T>::value> {};

template <class T>
inline constexpr bool is_unsigned_v = is_unsigned<T>::value;

template <class T>
struct is_abstract : integral_constant<bool, __is_abstract(T)> {};

template <class T>
inline constexpr bool is_abstract_v = is_abstract<T>::value;

template <class T>
struct is_aggregate : integral_constant<bool, __is_aggregate(T)> {};

template <class T>
inline constexpr bool is_aggregate_v = is_aggregate<T>::value;

template <class T>
struct is_empty : integral_constant<bool, __is_empty(T)> {};

template <class T>
inline constexpr bool is_empty_v = is_empty<T>::value;

template <class T>
struct is_polymorphic : integral_constant<bool, __is_polymorphic(T)> {};

template <class T>
inline constexpr bool is_polymorphic_v = is_polymorphic<T>::value;

template <class T>
struct is_final : integral_constant<bool, __is_final(T)> {};

template <class T>
inline constexpr bool is_final_v = is_final<T>::value;

template <class T>
struct is_bounded_array : false_type {};

template <class T, size_t N>
struct is_bounded_array<T[N]> : true_type {};

template <class T>
inline constexpr bool is_bounded_array_v = is_bounded_array<T>::value;

template <class T>
struct is_unbounded_array : false_type {};

template <class T>
struct is_unbounded_array<T[]> : true_type {};

template <class T>
inline constexpr bool is_unbounded_array_v = is_unbounded_array<T>::value;

template <class T>
struct is_nothrow_swappable
    : integral_constant<bool, noexcept(swap(declval<T&>(), declval<T&>()))> {};

template <class T>
inline constexpr bool is_nothrow_swappable_v = is_nothrow_swappable<T>::value;

template <class T, class U>
struct is_nothrow_swappable_with
    : integral_constant<bool, noexcept(swap(declval<T>(), declval<U>())) &&
                                  noexcept(swap(declval<U>(), declval<T>()))> {};

template <class T, class U>
inline constexpr bool is_nothrow_swappable_with_v = is_nothrow_swappable_with<T, U>::value;

template <class T>
struct is_swappable : integral_constant<bool, noexcept(swap(declval<T&>(), declval<T&>()))> {};

template <class T>
inline constexpr bool is_swappable_v = is_swappable<T>::value;

template <class T, class U>
struct is_swappable_with : integral_constant<bool, noexcept(swap(declval<T>(), declval<U>()))> {};

template <class T, class U>
inline constexpr bool is_swappable_with_v = is_swappable_with<T, U>::value;

template <class T>
struct has_unique_object_representations
    : integral_constant<bool, __has_unique_object_representations(T)> {};

template <class T>
inline constexpr bool has_unique_object_representations_v =
    has_unique_object_representations<T>::value;

template <class T>
struct is_invocable : integral_constant<bool, noexcept(declval<T>()())> {};

template <class T, class... Args>
struct is_invocable_r;

template <class R, class T, class... Args>
struct is_invocable_r<R, T, Args...>
    : integral_constant<bool, noexcept(declval<T>()(declval<Args>()...))> {};

template <class R, class T, class... Args>
inline constexpr bool is_invocable_r_v = is_invocable_r<R, T, Args...>::value;

template <class T>
inline constexpr bool is_invocable_v = is_invocable<T>::value;

template <class R, class T, class... Args>
inline constexpr bool is_nothrow_invocable_r_v = is_invocable_r<R, T, Args...>::value;

template <class T>
inline constexpr bool is_nothrow_invocable_v = false;

template <class T>
struct invoke_result;

template <class T>
struct invoke_result {
  using type = decltype(declval<T>()());
};

template <class T, class... Args>
struct invoke_result<T(Args...)> {
  using type = decltype(declval<T>()(declval<Args>()...));
};

template <class T>
using invoke_result_t = typename invoke_result<T>::type;

template <class T>
struct result_of;

template <class T>
struct result_of<T()> {
  using type = decltype(declval<T>()());
};

template <class T, class... Args>
struct result_of<T(Args...)> {
  using type = decltype(declval<T>()(declval<Args>()...));
};

template <class T>
using result_of_t = typename result_of<T>::type;

template <class T>
T* addressof(T& v) noexcept {
  return __builtin_addressof(v);
}

template <class T>
constexpr const T* addressof(const T&&) = delete;

template <class T, class U>
constexpr bool cmp_not_equal(T t, U u) noexcept {
  if constexpr (is_signed<T>::value == is_signed<U>::value) {
    return t != u;
  } else if constexpr (is_signed<T>::value) {
    // T is signed, U is unsigned: negative T can never equal U.
    return t < 0 || static_cast<U>(t) != u;
  } else {
    // T is unsigned, U is signed: negative U can never equal T.
    return u < 0 || t != static_cast<T>(u);
  }
}

}  // namespace std

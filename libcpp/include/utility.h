#pragma once

#include <cstddef.h>
#include <type_traits.h>

namespace std {

    template <typename T>
    constexpr T &&forward(typename std::remove_reference<T>::type &t) noexcept {
        return static_cast<T &&>(t);
    }

    template <typename T>
    constexpr T &&forward(
        typename std::remove_reference<T>::type &&t) noexcept {
        static_assert(!std::is_lvalue_reference<T>::value,
                      "Cannot forward an rvalue as an lvalue");
        return static_cast<T &&>(t);
    }

    template <typename T>
    constexpr typename std::remove_reference<T>::type &&move(T &&t) noexcept {
        return static_cast<typename std::remove_reference<T>::type &&>(t);
    }

    template <typename T>
    constexpr void swap(T &a, T &b) noexcept {
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

    template <typename...>
    using void_t = void;

    template <typename T>
    struct is_nothrow_default_constructible
        : integral_constant<bool, noexcept(T())> {};

    template <typename T>
    inline constexpr bool is_nothrow_default_constructible_v =
        is_nothrow_default_constructible<T>::value;

    template <typename T>
    struct is_nothrow_copy_constructible
        : integral_constant<bool, noexcept(T(std::declval<const T &>()))> {};

    template <typename T>
    inline constexpr bool is_nothrow_copy_constructible_v =
        is_nothrow_copy_constructible<T>::value;

    template <typename T>
    struct is_nothrow_move_constructible
        : integral_constant<bool, noexcept(T(std::declval<T &&>()))> {};

    template <typename T>
    inline constexpr bool is_nothrow_move_constructible_v =
        is_nothrow_move_constructible<T>::value;

    template <typename T>
    struct is_copy_assignable
        : integral_constant<bool, noexcept(std::declval<T &>() =
                                               std::declval<const T &>())> {};

    template <typename T>
    inline constexpr bool is_copy_assignable_v = is_copy_assignable<T>::value;

    template <typename T>
    struct is_move_assignable
        : integral_constant<bool, noexcept(std::declval<T &>() =
                                               std::declval<T &&>())> {};

    template <typename T>
    inline constexpr bool is_move_assignable_v = is_move_assignable<T>::value;

    template <class T>
    struct is_trivially_destructible
        : integral_constant<bool, __has_trivial_destructor(T)> {};

    template <class T>
    inline constexpr bool is_trivially_destructible_v =
        is_trivially_destructible<T>::value;

    template <class T>
    struct is_trivially_copyable
        : integral_constant<bool, __is_trivially_copyable(T)> {};

    template <class T>
    inline constexpr bool is_trivially_copyable_v =
        is_trivially_copyable<T>::value;

    template <class T>
    struct is_trivially_constructible
        : integral_constant<bool, __is_trivially_constructible(T)> {};

    template <class T>
    inline constexpr bool is_trivially_constructible_v =
        is_trivially_constructible<T>::value;

    template <class T>
    struct is_trivially_assignable
        : integral_constant<bool, __is_trivially_assignable(T, T)> {};

    template <class T>
    inline constexpr bool is_trivially_assignable_v =
        is_trivially_assignable<T>::value;

    template <typename T>
    struct is_nothrow_destructible
        : integral_constant<bool, noexcept(~declval<T>())> {};

    template <typename T>
    inline constexpr bool is_nothrow_destructible_v =
        is_nothrow_destructible<T>::value;

    template <class T, class U>
    struct is_assignable
        : integral_constant<bool,
                            noexcept(std::declval<T>() = std::declval<U>())> {};

    template <class T, class U>
    inline constexpr bool is_assignable_v = is_assignable<T, U>::value;

    template <class T>
    struct is_destructible : integral_constant<bool, noexcept(~declval<T>())> {
    };

    template <class T>
    inline constexpr bool is_destructible_v = is_destructible<T>::value;

    template <class T, class... Args>
    struct is_constructible
        : integral_constant<bool, noexcept(T(declval<Args>()...))> {};

    template <class T, class... Args>
    inline constexpr bool is_constructible_v =
        is_constructible<T, Args...>::value;

    template <class T>
    struct is_default_constructible : is_constructible<T> {};

    template <class T>
    inline constexpr bool is_default_constructible_v =
        is_default_constructible<T>::value;

    template <class T>
    struct is_copy_constructible : is_constructible<T, const T &> {};

    template <class T>
    inline constexpr bool is_copy_constructible_v =
        is_copy_constructible<T>::value;

    template <class T>
    struct is_move_constructible : is_constructible<T, T &&> {};

    template <class T>
    inline constexpr bool is_move_constructible_v =
        is_move_constructible<T>::value;

    template <class T>
    struct is_pod
        : integral_constant<bool, is_trivially_destructible<T>::value
                                      && is_trivially_copyable<T>::value
                                      && is_trivially_constructible<T>::value> {
    };

    template <class T>
    inline constexpr bool is_pod_v = is_pod<T>::value;

    template <class T>
    struct is_literal_type : integral_constant<bool, __is_literal_type(T)> {};

    template <class T>
    inline constexpr bool is_literal_type_v = is_literal_type<T>::value;

    template <class T>
    struct is_standard_layout
        : integral_constant<bool, __is_standard_layout(T)> {};

    template <class T>
    inline constexpr bool is_standard_layout_v = is_standard_layout<T>::value;

    template <class T>
    struct is_trivial : integral_constant<bool, __is_trivial(T)> {};

    template <class T>
    inline constexpr bool is_trivial_v = is_trivial<T>::value;

    template <class T>
    struct is_signed
        : integral_constant<bool, (static_cast<T>(-1) < static_cast<T>(0))> {};

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
        : integral_constant<bool,
                            noexcept(swap(declval<T &>(), declval<T &>()))> {};

    template <class T>
    inline constexpr bool is_nothrow_swappable_v =
        is_nothrow_swappable<T>::value;

    template <class T, class U>
    struct is_nothrow_swappable_with
        : integral_constant<bool,
                            noexcept(swap(declval<T>(), declval<U>()))
                                && noexcept(swap(declval<U>(), declval<T>()))> {
    };

    template <class T, class U>
    inline constexpr bool is_nothrow_swappable_with_v =
        is_nothrow_swappable_with<T, U>::value;

    template <class T>
    struct is_swappable
        : integral_constant<bool,
                            noexcept(swap(declval<T &>(), declval<T &>()))> {};

    template <class T>
    inline constexpr bool is_swappable_v = is_swappable<T>::value;

    template <class T, class U>
    struct is_swappable_with
        : integral_constant<bool, noexcept(swap(declval<T>(), declval<U>()))> {
    };

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
        : integral_constant<bool, noexcept(declval<T>()(declval<Args>()...))> {
    };

    template <class R, class T, class... Args>
    inline constexpr bool is_invocable_r_v =
        is_invocable_r<R, T, Args...>::value;

    template <class T>
    inline constexpr bool is_invocable_v = is_invocable<T>::value;

    template <class R, class T, class... Args>
    inline constexpr bool is_nothrow_invocable_r_v =
        is_invocable_r<R, T, Args...>::value;

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
    T *addressof(T &v) noexcept {
        return __builtin_addressof(v);
    }

    template <class T>
    constexpr const T *addressof(const T &&) = delete;

}  // namespace std

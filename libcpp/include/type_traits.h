#pragma once

#include <cstddef.h>

namespace std {

    template <typename T, T v>
    struct integral_constant {
        static constexpr T value = v;
        using value_type = T;
        using type = integral_constant<T, v>;
        constexpr operator T() const noexcept {
            return value;
        }
        constexpr T operator()() const noexcept {
            return value;
        }
    };

    using true_type = integral_constant<bool, true>;
    using false_type = integral_constant<bool, false>;

    template <typename T>
    struct is_const : false_type {};

    template <typename T>
    struct is_const<const T> : true_type {};

    template <typename T>
    inline constexpr bool is_const_v = is_const<T>::value;

    template <typename T>
    struct is_volatile : false_type {};

    template <typename T>
    struct is_volatile<volatile T> : true_type {};

    template <typename T>
    inline constexpr bool is_volatile_v = is_volatile<T>::value;

    template <typename T>
    struct is_pointer : false_type {};

    template <typename T>
    struct is_pointer<T *> : true_type {};

    template <typename T>
    inline constexpr bool is_pointer_v = is_pointer<T>::value;

    template <typename T>
    struct is_reference : false_type {};

    template <typename T>
    struct is_reference<T &> : true_type {};

    template <typename T>
    struct is_reference<T &&> : true_type {};

    template <typename T>
    inline constexpr bool is_reference_v = is_reference<T>::value;

    template <typename T>
    struct is_lvalue_reference : false_type {};

    template <typename T>
    struct is_lvalue_reference<T &> : true_type {};

    template <typename T>
    inline constexpr bool is_lvalue_reference_v = is_lvalue_reference<T>::value;

    template <typename T>
    struct is_rvalue_reference : false_type {};

    template <typename T>
    struct is_rvalue_reference<T &&> : true_type {};

    template <typename T>
    inline constexpr bool is_rvalue_reference_v = is_rvalue_reference<T>::value;

    template <typename T>
    struct is_void : false_type {};

    template <>
    struct is_void<void> : true_type {};

    template <typename T>
    inline constexpr bool is_void_v = is_void<T>::value;

    template <typename T>
    struct is_integral : false_type {};

    template <>
    struct is_integral<bool> : true_type {};
    template <>
    struct is_integral<char> : true_type {};
    template <>
    struct is_integral<signed char> : true_type {};
    template <>
    struct is_integral<unsigned char> : true_type {};
    template <>
    struct is_integral<short> : true_type {};
    template <>
    struct is_integral<unsigned short> : true_type {};
    template <>
    struct is_integral<int> : true_type {};
    template <>
    struct is_integral<unsigned int> : true_type {};
    template <>
    struct is_integral<long> : true_type {};
    template <>
    struct is_integral<unsigned long> : true_type {};
    template <>
    struct is_integral<long long> : true_type {};
    template <>
    struct is_integral<unsigned long long> : true_type {};

    template <typename T>
    inline constexpr bool is_integral_v = is_integral<T>::value;

    template <typename T>
    struct is_floating_point : false_type {};

    template <>
    struct is_floating_point<float> : true_type {};
    template <>
    struct is_floating_point<double> : true_type {};
    template <>
    struct is_floating_point<long double> : true_type {};

    template <typename T>
    inline constexpr bool is_floating_point_v = is_floating_point<T>::value;

    template <typename T>
    struct is_array : false_type {};

    template <typename T>
    struct is_array<T[]> : true_type {};

    template <typename T, size_t N>
    struct is_array<T[N]> : true_type {};

    template <typename T>
    inline constexpr bool is_array_v = is_array<T>::value;

    template <typename T>
    struct is_enum : integral_constant<bool, __is_enum(T)> {};

    template <typename T>
    inline constexpr bool is_enum_v = is_enum<T>::value;

    template <typename T>
    struct is_union : integral_constant<bool, __is_union(T)> {};

    template <typename T>
    inline constexpr bool is_union_v = is_union<T>::value;

    template <typename T>
    struct is_class : integral_constant<bool, __is_class(T)> {};

    template <typename T>
    inline constexpr bool is_class_v = is_class<T>::value;

    template <typename T>
    struct is_function : integral_constant<bool, __is_function(T)> {};

    template <typename T>
    inline constexpr bool is_function_v = is_function<T>::value;

    template <typename T>
    struct is_object : integral_constant<bool, !is_function<T>::value
                                                   && !is_reference<T>::value> {
    };

    template <typename T>
    inline constexpr bool is_object_v = is_object<T>::value;

    template <typename T>
    struct is_member_pointer : false_type {};

    template <typename T, typename C>
    struct is_member_pointer<T C::*> : true_type {};

    template <typename T, typename C>
    struct is_member_pointer<T C::*const> : true_type {};

    template <typename T>
    inline constexpr bool is_member_pointer_v = is_member_pointer<T>::value;

    template <typename T>
    struct is_null_pointer : false_type {};

    template <>
    struct is_null_pointer<std::nullptr_t> : true_type {};

    template <typename T>
    inline constexpr bool is_null_pointer_v = is_null_pointer<T>::value;

    template <typename T>
    struct is_scalar
        : integral_constant<
              bool, is_integral<T>::value || is_floating_point<T>::value
                        || is_pointer<T>::value || is_member_pointer<T>::value
                        || is_null_pointer<T>::value> {};

    template <typename T>
    inline constexpr bool is_scalar_v = is_scalar<T>::value;

    template <typename T>
    struct remove_const {
        using type = T;
    };

    template <typename T>
    struct remove_const<const T> {
        using type = T;
    };

    template <typename T>
    using remove_const_t = typename remove_const<T>::type;

    template <typename T>
    struct remove_volatile {
        using type = T;
    };

    template <typename T>
    struct remove_volatile<volatile T> {
        using type = T;
    };

    template <typename T>
    using remove_volatile_t = typename remove_volatile<T>::type;

    template <typename T>
    struct remove_cv {
        using type =
            typename remove_volatile<typename remove_const<T>::type>::type;
    };

    template <typename T>
    using remove_cv_t = typename remove_cv<T>::type;

    template <typename T>
    struct add_const {
        using type = const T;
    };

    template <typename T>
    using add_const_t = typename add_const<T>::type;

    template <typename T>
    struct add_volatile {
        using type = volatile T;
    };

    template <typename T>
    using add_volatile_t = typename add_volatile<T>::type;

    template <typename T>
    struct add_cv {
        using type = volatile const T;
    };

    template <typename T>
    using add_cv_t = typename add_cv<T>::type;

    template <typename T>
    struct remove_pointer {
        using type = T;
    };

    template <typename T>
    struct remove_pointer<T *> {
        using type = T;
    };

    template <typename T>
    using remove_pointer_t = typename remove_pointer<T>::type;

    template <typename T>
    struct add_pointer {
        using type = T *;
    };

    template <typename T>
    using add_pointer_t = typename add_pointer<T>::type;

    template <typename T>
    struct remove_reference {
        using type = T;
    };

    template <typename T>
    struct remove_reference<T &> {
        using type = T;
    };

    template <typename T>
    struct remove_reference<T &&> {
        using type = T;
    };

    template <typename T>
    using remove_reference_t = typename remove_reference<T>::type;

    template <typename T>
    struct add_lvalue_reference {
        using type = T &;
    };

    template <typename T>
    using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

    template <typename T>
    struct add_rvalue_reference {
        using type = T &&;
    };

    template <typename T>
    using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

    template <typename T>
    struct remove_extent {
        using type = T;
    };

    template <typename T>
    struct remove_extent<T[]> {
        using type = T;
    };

    template <typename T, size_t N>
    struct remove_extent<T[N]> {
        using type = T;
    };

    template <typename T>
    using remove_extent_t = typename remove_extent<T>::type;

    template <typename T>
    struct remove_all_extents {
        using type = T;
    };

    template <typename T>
    struct remove_all_extents<T[]> {
        using type = typename remove_all_extents<T>::type;
    };

    template <typename T, size_t N>
    struct remove_all_extents<T[N]> {
        using type = typename remove_all_extents<T>::type;
    };

    template <typename T>
    using remove_all_extents_t = typename remove_all_extents<T>::type;

    template <bool B, typename T, typename F>
    struct conditional {
        using type = T;
    };

    template <typename T, typename F>
    struct conditional<false, T, F> {
        using type = F;
    };

    template <bool B, typename T, typename F>
    using conditional_t = typename conditional<B, T, F>::type;

    template <bool B, typename T = void>
    struct enable_if {};

    template <typename T>
    struct enable_if<true, T> {
        using type = T;
    };

    template <bool B, typename T = void>
    using enable_if_t = typename enable_if<B, T>::type;

    template <typename T>
    struct decay {
       private:
        using U = typename remove_reference<T>::type;

       public:
        using type = typename conditional<
            is_array<U>::value, typename remove_extent<U>::type *,
            typename conditional<is_function<U>::value,
                                 typename add_pointer<U>::type,
                                 typename remove_cv<U>::type>::type>::type;
    };

    template <typename T>
    using decay_t = typename decay<T>::type;

    // Forward declaration so common_type can use it.
    template <typename T>
    T &&declval() noexcept;

    template <typename... T>
    struct common_type {};

    template <typename T>
    struct common_type<T> {
        using type = T;
    };

    template <typename T, typename U>
    struct common_type<T, U> {
        using type = decltype(false ? declval<T>() : declval<U>());
    };

    template <typename T, typename U, typename... V>
    struct common_type<T, U, V...> {
        using type =
            typename common_type<typename common_type<T, U>::type, V...>::type;
    };

    template <typename... T>
    using common_type_t = typename common_type<T...>::type;

    template <typename T>
    struct underlying_type {
        using type = __underlying_type(T);
    };

    template <typename T>
    using underlying_type_t = typename underlying_type<T>::type;

    template <typename T>
    struct make_unsigned {
       private:
        using U = typename remove_cv<T>::type;

       public:
        using type = typename conditional<
            sizeof(U) == 1, unsigned char,
            typename conditional<
                sizeof(U) == 2, unsigned short,
                typename conditional<
                    sizeof(U) == 4, unsigned int,
                    typename conditional<sizeof(U) == 8, unsigned long,
                                         unsigned long long>::type>::type>::
                type>::type;
    };

    template <typename T>
    using make_unsigned_t = typename make_unsigned<T>::type;

    template <typename T>
    struct make_signed {
       private:
        using U = typename remove_cv<T>::type;

       public:
        using type = typename conditional<
            sizeof(U) == 1, signed char,
            typename conditional<
                sizeof(U) == 2, short,
                typename conditional<
                    sizeof(U) == 4, int,
                    typename conditional<sizeof(U) == 8, long,
                                         long long>::type>::type>::type>::type;
    };

    template <typename T>
    using make_signed_t = typename make_signed<T>::type;

    template <typename T>
    struct rank : integral_constant<size_t, 0> {};

    template <typename T>
    struct rank<T[]> : integral_constant<size_t, rank<T>::value + 1> {};

    template <typename T, size_t N>
    struct rank<T[N]> : integral_constant<size_t, rank<T>::value + 1> {};

    template <typename T>
    inline constexpr size_t rank_v = rank<T>::value;

    template <typename T, unsigned I = 0>
    struct extent : integral_constant<size_t, 0> {};

    template <typename T, unsigned I>
    struct extent<T[], I>
        : integral_constant<size_t, I == 0 ? 0 : extent<T, I - 1>::value> {};

    template <typename T, size_t N, unsigned I>
    struct extent<T[N], I>
        : integral_constant<size_t, I == 0 ? N : extent<T, I - 1>::value> {};

    template <typename T, unsigned I = 0>
    inline constexpr size_t extent_v = extent<T, I>::value;

    template <typename T, typename U>
    struct is_same : false_type {};

    template <typename T>
    struct is_same<T, T> : true_type {};

    template <typename T, typename U>
    inline constexpr bool is_same_v = is_same<T, U>::value;

    template <typename Base, typename Derived>
    struct is_base_of : integral_constant<bool, __is_base_of(Base, Derived)> {};

    template <typename Base, typename Derived>
    inline constexpr bool is_base_of_v = is_base_of<Base, Derived>::value;

    template <typename From, typename To>
    struct is_convertible
        : integral_constant<bool, __is_convertible(From, To)> {};

    template <typename From, typename To>
    inline constexpr bool is_convertible_v = is_convertible<From, To>::value;

    template <typename T>
    T &&declval() noexcept;

    template <typename T>
    struct alignment_of : integral_constant<size_t, alignof(T)> {};

    template <typename T>
    inline constexpr size_t alignment_of_v = alignment_of<T>::value;

    template <typename T>
    struct aligned_storage {
        struct type {
            alignas(T) unsigned char __data[sizeof(T)];
        };
    };

    template <typename T>
    using aligned_storage_t = typename aligned_storage<T>::type;

}  // namespace std

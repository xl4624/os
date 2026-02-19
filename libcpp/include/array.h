#pragma once

#include <cstddef.h>

namespace std {

    // _data uses max(N,1) so array<T,0> has valid (but uncallable) accessors
    // without a separate partial specialisation
    template <typename T, size_t N>
    struct array {
        T _data[N > 0 ? N : 1];

        using value_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = T &;
        using const_reference = const T &;
        using pointer = T *;
        using const_pointer = const T *;
        using iterator = T *;
        using const_iterator = const T *;

        constexpr reference operator[](size_type pos) {
            return _data[pos];
        }
        constexpr const_reference operator[](size_type pos) const {
            return _data[pos];
        }
        constexpr reference at(size_type pos) {
            return _data[pos];
        }
        constexpr const_reference at(size_type pos) const {
            return _data[pos];
        }
        constexpr reference front() {
            return _data[0];
        }
        constexpr const_reference front() const {
            return _data[0];
        }
        constexpr reference back() {
            return _data[N - 1];
        }
        constexpr const_reference back() const {
            return _data[N - 1];
        }
        constexpr T *data() noexcept {
            return N > 0 ? _data : nullptr;
        }
        constexpr const T *data() const noexcept {
            return N > 0 ? _data : nullptr;
        }

        constexpr iterator begin() noexcept {
            return N > 0 ? _data : nullptr;
        }
        constexpr const_iterator begin() const noexcept {
            return N > 0 ? _data : nullptr;
        }
        constexpr const_iterator cbegin() const noexcept {
            return N > 0 ? _data : nullptr;
        }
        constexpr iterator end() noexcept {
            return N > 0 ? _data + N : nullptr;
        }
        constexpr const_iterator end() const noexcept {
            return N > 0 ? _data + N : nullptr;
        }
        constexpr const_iterator cend() const noexcept {
            return N > 0 ? _data + N : nullptr;
        }

        constexpr bool empty() const noexcept {
            return N == 0;
        }
        constexpr size_type size() const noexcept {
            return N;
        }
        constexpr size_type max_size() const noexcept {
            return N;
        }

        constexpr void fill(const T &value) {
            for (size_type i = 0; i < N; ++i) {
                _data[i] = value;
            }
        }

        constexpr void swap(array &other) noexcept {
            for (size_type i = 0; i < N; ++i) {
                T tmp = _data[i];
                _data[i] = other._data[i];
                other._data[i] = tmp;
            }
        }
    };

}  // namespace std

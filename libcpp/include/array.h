#pragma once

#include <cstddef.h>

namespace std {

    template <typename T, size_t N>
    struct array {
        T _data[N];

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
            return _data;
        }
        constexpr const T *data() const noexcept {
            return _data;
        }

        constexpr iterator begin() noexcept {
            return _data;
        }
        constexpr const_iterator begin() const noexcept {
            return _data;
        }
        constexpr const_iterator cbegin() const noexcept {
            return _data;
        }
        constexpr iterator end() noexcept {
            return _data + N;
        }
        constexpr const_iterator end() const noexcept {
            return _data + N;
        }
        constexpr const_iterator cend() const noexcept {
            return _data + N;
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

    template <typename T>
    struct array<T, 0> {
        using value_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = T &;
        using const_reference = const T &;
        using pointer = T *;
        using const_pointer = const T *;
        using iterator = T *;
        using const_iterator = const T *;

        constexpr reference operator[](size_type) {
            return *_data;
        }
        constexpr const_reference operator[](size_type) const {
            return *_data;
        }
        constexpr reference at(size_type) {
            return *_data;
        }
        constexpr const_reference at(size_type) const {
            return *_data;
        }
        constexpr reference front() {
            return *_data;
        }
        constexpr const_reference front() const {
            return *_data;
        }
        constexpr reference back() {
            return *_data;
        }
        constexpr const_reference back() const {
            return *_data;
        }
        constexpr T *data() noexcept {
            return nullptr;
        }
        constexpr const T *data() const noexcept {
            return nullptr;
        }

        constexpr iterator begin() noexcept {
            return nullptr;
        }
        constexpr const_iterator begin() const noexcept {
            return nullptr;
        }
        constexpr const_iterator cbegin() const noexcept {
            return nullptr;
        }
        constexpr iterator end() noexcept {
            return nullptr;
        }
        constexpr const_iterator end() const noexcept {
            return nullptr;
        }
        constexpr const_iterator cend() const noexcept {
            return nullptr;
        }

        constexpr bool empty() const noexcept {
            return true;
        }
        constexpr size_type size() const noexcept {
            return 0;
        }
        constexpr size_type max_size() const noexcept {
            return 0;
        }

        constexpr void fill(const T &) {}
        constexpr void swap(array &) noexcept {}
    };

}  // namespace std


#pragma once

#include <cstddef.h>
#include <type_traits.h>
#include <utility.h>

namespace std {

// Default deleter for single objects.
template <typename T>
struct default_delete {
  constexpr default_delete() noexcept = default;

  template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
  constexpr default_delete(const default_delete<U>&) noexcept {}

  void operator()(T* ptr) const noexcept {
    static_assert(!is_void_v<T>, "Cannot delete a void pointer");
    delete ptr;
  }
};

// Default deleter for arrays.
template <typename T>
struct default_delete<T[]> {
  constexpr default_delete() noexcept = default;

  void operator()(T* ptr) const noexcept { delete[] ptr; }
};

// unique_ptr for single objects.
template <typename T, typename Deleter = default_delete<T>>
class unique_ptr {
 public:
  using pointer = T*;
  using element_type = T;
  using deleter_type = Deleter;

  constexpr unique_ptr() noexcept : ptr_(nullptr) {}
  constexpr unique_ptr(nullptr_t) noexcept : ptr_(nullptr) {}
  explicit unique_ptr(pointer p) noexcept : ptr_(p) {}
  unique_ptr(pointer p, const Deleter& d) noexcept : ptr_(p), deleter_(d) {}
  unique_ptr(pointer p, Deleter&& d) noexcept : ptr_(p), deleter_(std::move(d)) {}

  unique_ptr(unique_ptr&& other) noexcept
      : ptr_(other.release()), deleter_(std::move(other.deleter_)) {}

  template <typename U, typename E,
            enable_if_t<is_convertible_v<typename unique_ptr<U, E>::pointer, pointer>, int> = 0>
  unique_ptr(unique_ptr<U, E>&& other) noexcept
      : ptr_(other.release()), deleter_(std::forward<E>(other.get_deleter())) {}

  ~unique_ptr() noexcept {
    if (ptr_) deleter_(ptr_);
  }

  unique_ptr(const unique_ptr&) = delete;
  unique_ptr& operator=(const unique_ptr&) = delete;

  unique_ptr& operator=(unique_ptr&& other) noexcept {
    if (this != &other) {
      reset(other.release());
      deleter_ = std::move(other.deleter_);
    }
    return *this;
  }

  unique_ptr& operator=(nullptr_t) noexcept {
    reset();
    return *this;
  }

  [[nodiscard]] pointer get() const noexcept { return ptr_; }
  [[nodiscard]] Deleter& get_deleter() noexcept { return deleter_; }
  [[nodiscard]] const Deleter& get_deleter() const noexcept { return deleter_; }

  [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

  typename add_lvalue_reference<T>::type operator*() const noexcept { return *ptr_; }
  pointer operator->() const noexcept { return ptr_; }

  [[nodiscard]] pointer release() noexcept {
    pointer p = ptr_;
    ptr_ = nullptr;
    return p;
  }

  void reset(pointer p = nullptr) noexcept {
    pointer old = ptr_;
    ptr_ = p;
    if (old) deleter_(old);
  }

  void swap(unique_ptr& other) noexcept {
    std::swap(ptr_, other.ptr_);
    std::swap(deleter_, other.deleter_);
  }

 private:
  pointer ptr_;
  Deleter deleter_;
};

// unique_ptr specialization for arrays.
template <typename T, typename Deleter>
class unique_ptr<T[], Deleter> {
 public:
  using pointer = T*;
  using element_type = T;
  using deleter_type = Deleter;

  constexpr unique_ptr() noexcept : ptr_(nullptr) {}
  constexpr unique_ptr(nullptr_t) noexcept : ptr_(nullptr) {}
  explicit unique_ptr(pointer p) noexcept : ptr_(p) {}
  unique_ptr(pointer p, const Deleter& d) noexcept : ptr_(p), deleter_(d) {}
  unique_ptr(pointer p, Deleter&& d) noexcept : ptr_(p), deleter_(std::move(d)) {}

  unique_ptr(unique_ptr&& other) noexcept
      : ptr_(other.release()), deleter_(std::move(other.deleter_)) {}

  ~unique_ptr() noexcept {
    if (ptr_) deleter_(ptr_);
  }

  unique_ptr(const unique_ptr&) = delete;
  unique_ptr& operator=(const unique_ptr&) = delete;

  unique_ptr& operator=(unique_ptr&& other) noexcept {
    if (this != &other) {
      reset(other.release());
      deleter_ = std::move(other.deleter_);
    }
    return *this;
  }

  unique_ptr& operator=(nullptr_t) noexcept {
    reset();
    return *this;
  }

  [[nodiscard]] pointer get() const noexcept { return ptr_; }
  [[nodiscard]] Deleter& get_deleter() noexcept { return deleter_; }
  [[nodiscard]] const Deleter& get_deleter() const noexcept { return deleter_; }

  [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

  T& operator[](size_t i) const noexcept { return ptr_[i]; }

  [[nodiscard]] pointer release() noexcept {
    pointer p = ptr_;
    ptr_ = nullptr;
    return p;
  }

  void reset(pointer p = nullptr) noexcept {
    pointer old = ptr_;
    ptr_ = p;
    if (old) deleter_(old);
  }

  void swap(unique_ptr& other) noexcept {
    std::swap(ptr_, other.ptr_);
    std::swap(deleter_, other.deleter_);
  }

 private:
  pointer ptr_;
  Deleter deleter_;
};

// make_unique for single objects.
template <typename T, typename... Args, enable_if_t<!is_array_v<T>, int> = 0>
unique_ptr<T> make_unique(Args&&... args) {
  return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// make_unique for unbounded arrays: make_unique<T[]>(n).
template <typename T, enable_if_t<is_unbounded_array_v<T>, int> = 0>
unique_ptr<T> make_unique(size_t n) {
  using Element = remove_extent_t<T>;
  return unique_ptr<T>(new Element[n]());
}

// Comparison operators.

template <typename T1, typename D1, typename T2, typename D2>
bool operator==(const unique_ptr<T1, D1>& lhs, const unique_ptr<T2, D2>& rhs) noexcept {
  return lhs.get() == rhs.get();
}

template <typename T1, typename D1, typename T2, typename D2>
bool operator!=(const unique_ptr<T1, D1>& lhs, const unique_ptr<T2, D2>& rhs) noexcept {
  return !(lhs == rhs);
}

template <typename T, typename D>
bool operator==(const unique_ptr<T, D>& ptr, nullptr_t) noexcept {
  return !ptr;
}

template <typename T, typename D>
bool operator==(nullptr_t, const unique_ptr<T, D>& ptr) noexcept {
  return !ptr;
}

template <typename T, typename D>
bool operator!=(const unique_ptr<T, D>& ptr, nullptr_t) noexcept {
  return static_cast<bool>(ptr);
}

template <typename T, typename D>
bool operator!=(nullptr_t, const unique_ptr<T, D>& ptr) noexcept {
  return static_cast<bool>(ptr);
}

}  // namespace std

#pragma once

#include <assert.h>
#include <new.h>
#include <type_traits.h>
#include <utility.h>

namespace std {

struct nullopt_t {
  struct _Tag {};
  explicit constexpr nullopt_t(_Tag) noexcept {}
};

inline constexpr nullopt_t nullopt{nullopt_t::_Tag{}};

// optional<T>: holds either a value of type T or nothing.
// Callers must check has_value() before accessing.
template <typename T>
class optional {
 public:
  using value_type = T;

  // Constructors

  constexpr optional() noexcept : has_value_(false) {}
  constexpr optional(nullopt_t) noexcept : has_value_(false) {}

  optional(const optional& other) : has_value_(other.has_value_) {
    if (has_value_) {
      new (ptr()) T(*other.ptr());
    }
  }

  optional(optional&& other) noexcept(is_nothrow_move_constructible_v<T>)
      : has_value_(other.has_value_) {
    if (has_value_) {
      new (ptr()) T(std::move(*other.ptr()));
      other.reset();
    }
  }

  // Converting constructor from a compatible value.
  template <typename U = T,
            enable_if_t<!is_same_v<remove_cv_t<remove_reference_t<U>>, optional>, int> = 0>
  optional(U&& value) : has_value_(true) {
    new (ptr()) T(std::forward<U>(value));
  }

  ~optional() { reset(); }

  // Assignment

  optional& operator=(nullopt_t) noexcept {
    reset();
    return *this;
  }

  optional& operator=(const optional& other) {
    if (this != &other) {
      reset();
      if (other.has_value_) {
        new (ptr()) T(*other.ptr());
        has_value_ = true;
      }
    }
    return *this;
  }

  optional& operator=(optional&& other) noexcept(is_nothrow_move_constructible_v<T>) {
    if (this != &other) {
      reset();
      if (other.has_value_) {
        new (ptr()) T(std::move(*other.ptr()));
        has_value_ = true;
        other.reset();
      }
    }
    return *this;
  }

  template <typename U = T,
            enable_if_t<!is_same_v<remove_cv_t<remove_reference_t<U>>, optional>, int> = 0>
  optional& operator=(U&& value) {
    reset();
    new (ptr()) T(std::forward<U>(value));
    has_value_ = true;
    return *this;
  }

  // Observers

  [[nodiscard]] constexpr bool has_value() const noexcept { return has_value_; }
  [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value_; }

  T& operator*() & noexcept { return *ptr(); }
  const T& operator*() const& noexcept { return *ptr(); }
  T&& operator*() && noexcept { return std::move(*ptr()); }
  T* operator->() noexcept { return ptr(); }
  const T* operator->() const noexcept { return ptr(); }

  T& value() & {
    assert(has_value_);
    return *ptr();
  }
  const T& value() const& {
    assert(has_value_);
    return *ptr();
  }
  T&& value() && {
    assert(has_value_);
    return std::move(*ptr());
  }

  template <typename U>
  T value_or(U&& default_value) const& {
    return has_value_ ? *ptr() : static_cast<T>(std::forward<U>(default_value));
  }

  template <typename U>
  T value_or(U&& default_value) && {
    return has_value_ ? std::move(*ptr()) : static_cast<T>(std::forward<U>(default_value));
  }

  // Modifiers

  template <typename... Args>
  T& emplace(Args&&... args) {
    reset();
    new (ptr()) T(std::forward<Args>(args)...);
    has_value_ = true;
    return *ptr();
  }

  void reset() noexcept {
    if (has_value_) {
      ptr()->~T();
      has_value_ = false;
    }
  }

  void swap(optional& other) noexcept(is_nothrow_move_constructible_v<T> &&
                                      is_nothrow_swappable_v<T>) {
    if (has_value_ && other.has_value_) {
      std::swap(*ptr(), *other.ptr());
    } else if (has_value_) {
      new (other.ptr()) T(std::move(*ptr()));
      other.has_value_ = true;
      reset();
    } else if (other.has_value_) {
      new (ptr()) T(std::move(*other.ptr()));
      has_value_ = true;
      other.reset();
    }
  }

 private:
  T* ptr() noexcept { return reinterpret_cast<T*>(storage_); }
  const T* ptr() const noexcept { return reinterpret_cast<const T*>(storage_); }

  alignas(T) unsigned char storage_[sizeof(T)];
  bool has_value_;
};

// Deduction guide
template <typename T>
optional(T) -> optional<T>;

// make_optional

template <typename T>
optional<remove_cv_t<remove_reference_t<T>>> make_optional(T&& value) {
  return optional<remove_cv_t<remove_reference_t<T>>>(std::forward<T>(value));
}

template <typename T, typename... Args>
optional<T> make_optional(Args&&... args) {
  optional<T> o;
  o.emplace(std::forward<Args>(args)...);
  return o;
}

// Comparison: two optionals

template <typename T>
constexpr bool operator==(const optional<T>& lhs, const optional<T>& rhs) {
  if (lhs.has_value() != rhs.has_value()) return false;
  if (!lhs.has_value()) return true;
  return *lhs == *rhs;
}

template <typename T>
constexpr bool operator!=(const optional<T>& lhs, const optional<T>& rhs) {
  return !(lhs == rhs);
}

// Comparison: optional vs nullopt

template <typename T>
constexpr bool operator==(const optional<T>& opt, nullopt_t) noexcept {
  return !opt.has_value();
}

template <typename T>
constexpr bool operator==(nullopt_t, const optional<T>& opt) noexcept {
  return !opt.has_value();
}

template <typename T>
constexpr bool operator!=(const optional<T>& opt, nullopt_t) noexcept {
  return opt.has_value();
}

template <typename T>
constexpr bool operator!=(nullopt_t, const optional<T>& opt) noexcept {
  return opt.has_value();
}

// Comparison: optional vs value

template <typename T, typename U>
constexpr bool operator==(const optional<T>& opt, const U& val) {
  return opt.has_value() && *opt == val;
}

template <typename T, typename U>
constexpr bool operator==(const U& val, const optional<T>& opt) {
  return opt.has_value() && *opt == val;
}

template <typename T, typename U>
constexpr bool operator!=(const optional<T>& opt, const U& val) {
  return !opt.has_value() || *opt != val;
}

template <typename T, typename U>
constexpr bool operator!=(const U& val, const optional<T>& opt) {
  return !opt.has_value() || *opt != val;
}

}  // namespace std

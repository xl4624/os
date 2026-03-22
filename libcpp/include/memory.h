#pragma once

#include <cstddef.h>
#include <new.h>
#include <type_traits.h>
#include <utility.h>

namespace std {

// Intrusive reference-counted control block.
namespace detail {

struct ControlBlock {
  size_t ref_count;

  explicit ControlBlock(size_t initial) : ref_count(initial) {}
  virtual ~ControlBlock() = default;
  virtual void destroy_object() = 0;

  void add_ref() { ++ref_count; }

  // Returns true if the object was destroyed.
  bool release() {
    if (--ref_count == 0) {
      destroy_object();
      return true;
    }
    return false;
  }
};

// Control block that owns a separately-allocated pointer.
template <typename T>
struct PointerControlBlock final : ControlBlock {
  T* ptr;

  explicit PointerControlBlock(T* p) : ControlBlock(1), ptr(p) {}

  void destroy_object() override { delete ptr; }
};

// Control block with the object embedded (for make_shared).
template <typename T>
struct InplaceControlBlock final : ControlBlock {
  alignas(T) char storage[sizeof(T)];

  template <typename... Args>
  explicit InplaceControlBlock(Args&&... args) : ControlBlock(1) {
    new (storage) T(std::forward<Args>(args)...);
  }

  T* get() { return reinterpret_cast<T*>(storage); }

  void destroy_object() override { get()->~T(); }
};

}  // namespace detail

template <typename T>
class shared_ptr {
 public:
  shared_ptr() noexcept : ptr_(nullptr), ctrl_(nullptr) {}
  shared_ptr(nullptr_t) noexcept : ptr_(nullptr), ctrl_(nullptr) {}

  explicit shared_ptr(T* p) : ptr_(p), ctrl_(nullptr) {
    if (p) {
      ctrl_ = new detail::PointerControlBlock<T>(p);
    }
  }

  shared_ptr(const shared_ptr& other) noexcept : ptr_(other.ptr_), ctrl_(other.ctrl_) {
    if (ctrl_) {
      ctrl_->add_ref();
    }
  }

  shared_ptr(shared_ptr&& other) noexcept : ptr_(other.ptr_), ctrl_(other.ctrl_) {
    other.ptr_ = nullptr;
    other.ctrl_ = nullptr;
  }

  // Aliasing constructor: shares ownership with other but points to p.
  template <typename U>
  shared_ptr(const shared_ptr<U>& other, T* p) noexcept : ptr_(p), ctrl_(other.ctrl_) {
    if (ctrl_) {
      ctrl_->add_ref();
    }
  }

  // Converting constructor from derived type.
  template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
  shared_ptr(const shared_ptr<U>& other) noexcept : ptr_(other.ptr_), ctrl_(other.ctrl_) {
    if (ctrl_) {
      ctrl_->add_ref();
    }
  }

  template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
  shared_ptr(shared_ptr<U>&& other) noexcept : ptr_(other.ptr_), ctrl_(other.ctrl_) {
    other.ptr_ = nullptr;
    other.ctrl_ = nullptr;
  }

  ~shared_ptr() {
    if (ctrl_ && ctrl_->release()) {
      delete ctrl_;
    }
  }

  shared_ptr& operator=(const shared_ptr& other) noexcept {
    if (this != &other) {
      shared_ptr tmp(other);
      swap(tmp);
    }
    return *this;
  }

  shared_ptr& operator=(shared_ptr&& other) noexcept {
    if (this != &other) {
      shared_ptr tmp(std::move(other));
      swap(tmp);
    }
    return *this;
  }

  shared_ptr& operator=(nullptr_t) noexcept {
    reset();
    return *this;
  }

  void reset() noexcept { shared_ptr().swap(*this); }

  void reset(T* p) { shared_ptr(p).swap(*this); }

  void swap(shared_ptr& other) noexcept {
    std::swap(ptr_, other.ptr_);
    std::swap(ctrl_, other.ctrl_);
  }

  [[nodiscard]] T* get() const noexcept { return ptr_; }
  [[nodiscard]] T& operator*() const noexcept { return *ptr_; }
  [[nodiscard]] T* operator->() const noexcept { return ptr_; }
  [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

  [[nodiscard]] size_t use_count() const noexcept { return ctrl_ ? ctrl_->ref_count : 0; }

  [[nodiscard]] bool unique() const noexcept { return use_count() == 1; }

 private:
  template <typename U>
  friend class shared_ptr;

  template <typename U, typename... Args>
  friend shared_ptr<U> make_shared(Args&&... args);

  T* ptr_;
  detail::ControlBlock* ctrl_;

  // Private constructor for make_shared.
  shared_ptr(T* p, detail::ControlBlock* c) noexcept : ptr_(p), ctrl_(c) {}
};

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
  auto* ctrl = new detail::InplaceControlBlock<T>(std::forward<Args>(args)...);
  return shared_ptr<T>(ctrl->get(), ctrl);
}

// Comparison operators.
template <typename T, typename U>
bool operator==(const shared_ptr<T>& a, const shared_ptr<U>& b) noexcept {
  return a.get() == b.get();
}

template <typename T, typename U>
bool operator!=(const shared_ptr<T>& a, const shared_ptr<U>& b) noexcept {
  return !(a == b);
}

template <typename T>
bool operator==(const shared_ptr<T>& p, nullptr_t) noexcept {
  return !p;
}

template <typename T>
bool operator==(nullptr_t, const shared_ptr<T>& p) noexcept {
  return !p;
}

template <typename T>
bool operator!=(const shared_ptr<T>& p, nullptr_t) noexcept {
  return static_cast<bool>(p);
}

template <typename T>
bool operator!=(nullptr_t, const shared_ptr<T>& p) noexcept {
  return static_cast<bool>(p);
}

}  // namespace std

#pragma once

#include <cstddef.h>
#include <new.h>
#include <type_traits.h>
#include <utility.h>

namespace std {

// Type-erased callable wrapper (no exceptions, no RTTI).
template <typename>
class function;

template <typename R, typename... Args>
class function<R(Args...)> {
 public:
  function() noexcept : callable_(nullptr) {}
  function(nullptr_t) noexcept : callable_(nullptr) {}

  function(const function& other) : callable_(nullptr) {
    if (other.callable_) {
      callable_ = other.callable_->clone(storage_);
    }
  }

  function(function&& other) noexcept : callable_(nullptr) {
    if (other.callable_) {
      callable_ = other.callable_->move_to(storage_);
      other.callable_->destroy();
      other.callable_ = nullptr;
    }
  }

  // Construct from any callable that fits the signature.
  template <typename F, enable_if_t<!is_same_v<decay_t<F>, function>, int> = 0>
  function(F&& f) : callable_(nullptr) {
    using Callable = decay_t<F>;
    if constexpr (sizeof(Model<Callable>) <= kSmallBufferSize) {
      callable_ = new (storage_) Model<Callable>(std::forward<F>(f));
    } else {
      callable_ = new Model<Callable>(std::forward<F>(f));
    }
  }

  ~function() {
    if (callable_) {
      callable_->destroy();
    }
  }

  function& operator=(const function& other) {
    if (this != &other) {
      function tmp(other);
      swap(tmp);
    }
    return *this;
  }

  function& operator=(function&& other) noexcept {
    if (this != &other) {
      if (callable_) {
        callable_->destroy();
      }
      callable_ = nullptr;
      if (other.callable_) {
        callable_ = other.callable_->move_to(storage_);
        other.callable_->destroy();
        other.callable_ = nullptr;
      }
    }
    return *this;
  }

  function& operator=(nullptr_t) noexcept {
    if (callable_) {
      callable_->destroy();
      callable_ = nullptr;
    }
    return *this;
  }

  R operator()(Args... args) const { return callable_->invoke(std::forward<Args>(args)...); }

  [[nodiscard]] explicit operator bool() const noexcept { return callable_ != nullptr; }

  void swap(function& other) noexcept {
    // Simple swap via moves through a temporary.
    function tmp(std::move(other));
    other = std::move(*this);
    *this = std::move(tmp);
  }

 private:
  // Abstract base for type-erased callables.
  struct Concept {
    virtual ~Concept() = default;
    virtual R invoke(Args... args) = 0;
    // Clone into the provided small buffer (or heap-allocate if too large).
    virtual Concept* clone(void* buf) const = 0;
    // Move into the provided small buffer (or heap-allocate if too large).
    virtual Concept* move_to(void* buf) = 0;
    // Destroy this object (calling destructor and freeing if heap-allocated).
    virtual void destroy() = 0;
  };

  template <typename F>
  struct Model final : Concept {
    F func_;

    explicit Model(const F& f) : func_(f) {}
    explicit Model(F&& f) : func_(std::move(f)) {}

    R invoke(Args... args) override { return func_(std::forward<Args>(args)...); }

    Concept* clone(void* buf) const override {
      if constexpr (sizeof(Model) <= kSmallBufferSize) {
        return new (buf) Model(func_);
      } else {
        return new Model(func_);
      }
    }

    Concept* move_to(void* buf) override {
      if constexpr (sizeof(Model) <= kSmallBufferSize) {
        return new (buf) Model(std::move(func_));
      } else {
        return new Model(std::move(func_));
      }
    }

    void destroy() override {
      bool is_small = (sizeof(Model) <= kSmallBufferSize);
      this->~Model();
      if (!is_small) {
        ::operator delete(this);
      }
    }
  };

  static constexpr size_t kSmallBufferSize = sizeof(void*) * 4;
  alignas(sizeof(void*)) char storage_[kSmallBufferSize];
  Concept* callable_;
};

}  // namespace std

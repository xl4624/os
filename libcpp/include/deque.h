#pragma once

#include <algorithm.h>
#include <cstddef.h>
#include <initializer_list.h>
#include <iterator.h>
#include <new.h>
#include <utility.h>

namespace std {

// Simple ring-buffer-based deque. Grows by doubling capacity.
template <typename T>
class deque {
 public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

  // Random-access iterator over the ring buffer.
  class iterator {
   public:
    using iterator_category = random_access_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    iterator() : deq_(nullptr), index_(0) {}
    iterator(deque* d, size_type i) : deq_(d), index_(i) {}

    reference operator*() const { return (*deq_)[index_]; }
    pointer operator->() const { return &(*deq_)[index_]; }

    iterator& operator++() {
      ++index_;
      return *this;
    }
    iterator operator++(int) {
      iterator tmp = *this;
      ++index_;
      return tmp;
    }
    iterator& operator--() {
      --index_;
      return *this;
    }
    iterator operator--(int) {
      iterator tmp = *this;
      --index_;
      return tmp;
    }

    iterator& operator+=(difference_type n) {
      index_ = static_cast<size_type>(static_cast<difference_type>(index_) + n);
      return *this;
    }
    iterator& operator-=(difference_type n) {
      index_ = static_cast<size_type>(static_cast<difference_type>(index_) - n);
      return *this;
    }
    iterator operator+(difference_type n) const {
      iterator tmp = *this;
      tmp += n;
      return tmp;
    }
    iterator operator-(difference_type n) const {
      iterator tmp = *this;
      tmp -= n;
      return tmp;
    }
    difference_type operator-(const iterator& other) const {
      return static_cast<difference_type>(index_) - static_cast<difference_type>(other.index_);
    }

    reference operator[](difference_type n) const {
      return (*deq_)[index_ + static_cast<size_type>(n)];
    }

    bool operator==(const iterator& o) const { return index_ == o.index_; }
    bool operator!=(const iterator& o) const { return index_ != o.index_; }
    bool operator<(const iterator& o) const { return index_ < o.index_; }
    bool operator>(const iterator& o) const { return index_ > o.index_; }
    bool operator<=(const iterator& o) const { return index_ <= o.index_; }
    bool operator>=(const iterator& o) const { return index_ >= o.index_; }

   private:
    deque* deq_;
    size_type index_;
  };

  class const_iterator {
   public:
    using iterator_category = random_access_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;

    const_iterator() : deq_(nullptr), index_(0) {}
    const_iterator(const deque* d, size_type i) : deq_(d), index_(i) {}
    const_iterator(iterator it) : deq_(it.deq_), index_(it.index_) {}  // NOLINT

    reference operator*() const { return (*deq_)[index_]; }
    pointer operator->() const { return &(*deq_)[index_]; }

    const_iterator& operator++() {
      ++index_;
      return *this;
    }
    const_iterator operator++(int) {
      const_iterator tmp = *this;
      ++index_;
      return tmp;
    }
    const_iterator& operator--() {
      --index_;
      return *this;
    }
    const_iterator operator--(int) {
      const_iterator tmp = *this;
      --index_;
      return tmp;
    }

    const_iterator& operator+=(difference_type n) {
      index_ = static_cast<size_type>(static_cast<difference_type>(index_) + n);
      return *this;
    }
    const_iterator operator+(difference_type n) const {
      const_iterator tmp = *this;
      tmp += n;
      return tmp;
    }
    difference_type operator-(const const_iterator& other) const {
      return static_cast<difference_type>(index_) - static_cast<difference_type>(other.index_);
    }

    bool operator==(const const_iterator& o) const { return index_ == o.index_; }
    bool operator!=(const const_iterator& o) const { return index_ != o.index_; }
    bool operator<(const const_iterator& o) const { return index_ < o.index_; }

   private:
    const deque* deq_;
    size_type index_;
  };

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // =========================================================================
  // Construction / destruction
  // =========================================================================

  deque() : data_(nullptr), capacity_(0), head_(0), size_(0) {}

  explicit deque(size_type n) : deque() {
    reserve(n);
    for (size_type i = 0; i < n; ++i) {
      push_back(T());
    }
  }

  deque(size_type n, const_reference value) : deque() {
    reserve(n);
    for (size_type i = 0; i < n; ++i) {
      push_back(value);
    }
  }

  deque(initializer_list<T> il) : deque() {
    reserve(il.size());
    for (const auto& v : il) {
      push_back(v);
    }
  }

  deque(const deque& other) : deque() {
    reserve(other.size_);
    for (size_type i = 0; i < other.size_; ++i) {
      push_back(other[i]);
    }
  }

  deque(deque&& other) noexcept
      : data_(other.data_), capacity_(other.capacity_), head_(other.head_), size_(other.size_) {
    other.data_ = nullptr;
    other.capacity_ = 0;
    other.head_ = 0;
    other.size_ = 0;
  }

  ~deque() {
    clear();
    ::operator delete(data_);
  }

  // =========================================================================
  // Assignment
  // =========================================================================

  deque& operator=(const deque& other) {
    if (this != &other) {
      deque tmp(other);
      swap(tmp);
    }
    return *this;
  }

  deque& operator=(deque&& other) noexcept {
    if (this != &other) {
      clear();
      ::operator delete(data_);
      data_ = other.data_;
      capacity_ = other.capacity_;
      head_ = other.head_;
      size_ = other.size_;
      other.data_ = nullptr;
      other.capacity_ = 0;
      other.head_ = 0;
      other.size_ = 0;
    }
    return *this;
  }

  // =========================================================================
  // Element access
  // =========================================================================

  reference operator[](size_type pos) { return data_[physical(pos)]; }
  const_reference operator[](size_type pos) const { return data_[physical(pos)]; }
  reference at(size_type pos) { return data_[physical(pos)]; }
  const_reference at(size_type pos) const { return data_[physical(pos)]; }
  reference front() { return data_[head_]; }
  const_reference front() const { return data_[head_]; }
  reference back() { return data_[physical(size_ - 1)]; }
  const_reference back() const { return data_[physical(size_ - 1)]; }

  // =========================================================================
  // Iterators
  // =========================================================================

  iterator begin() noexcept { return iterator(this, 0); }
  const_iterator begin() const noexcept { return const_iterator(this, 0); }
  const_iterator cbegin() const noexcept { return const_iterator(this, 0); }
  iterator end() noexcept { return iterator(this, size_); }
  const_iterator end() const noexcept { return const_iterator(this, size_); }
  const_iterator cend() const noexcept { return const_iterator(this, size_); }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

  // =========================================================================
  // Capacity
  // =========================================================================

  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
  [[nodiscard]] size_type size() const noexcept { return size_; }
  [[nodiscard]] size_type capacity() const noexcept { return capacity_; }

  void reserve(size_type new_cap) {
    if (new_cap > capacity_) {
      grow(new_cap);
    }
  }

  // =========================================================================
  // Modifiers
  // =========================================================================

  void clear() noexcept {
    for (size_type i = 0; i < size_; ++i) {
      data_[physical(i)].~T();
    }
    size_ = 0;
    head_ = 0;
  }

  void push_back(const_reference value) {
    ensure_capacity();
    new (&data_[physical(size_)]) T(value);
    ++size_;
  }

  void push_back(T&& value) {
    ensure_capacity();
    new (&data_[physical(size_)]) T(std::move(value));
    ++size_;
  }

  template <typename... Args>
  reference emplace_back(Args&&... args) {
    ensure_capacity();
    T* slot = &data_[physical(size_)];
    new (slot) T(std::forward<Args>(args)...);
    ++size_;
    return *slot;
  }

  void push_front(const_reference value) {
    ensure_capacity();
    head_ = (head_ == 0 ? capacity_ : head_) - 1;
    new (&data_[head_]) T(value);
    ++size_;
  }

  void push_front(T&& value) {
    ensure_capacity();
    head_ = (head_ == 0 ? capacity_ : head_) - 1;
    new (&data_[head_]) T(std::move(value));
    ++size_;
  }

  template <typename... Args>
  reference emplace_front(Args&&... args) {
    ensure_capacity();
    head_ = (head_ == 0 ? capacity_ : head_) - 1;
    new (&data_[head_]) T(std::forward<Args>(args)...);
    ++size_;
    return data_[head_];
  }

  void pop_back() {
    --size_;
    data_[physical(size_)].~T();
  }

  void pop_front() {
    data_[head_].~T();
    head_ = (head_ + 1) % capacity_;
    --size_;
  }

  void swap(deque& other) noexcept {
    std::swap(data_, other.data_);
    std::swap(capacity_, other.capacity_);
    std::swap(head_, other.head_);
    std::swap(size_, other.size_);
  }

  // =========================================================================
  // Comparison
  // =========================================================================

  friend bool operator==(const deque& a, const deque& b) {
    if (a.size_ != b.size_) {
      return false;
    }
    for (size_type i = 0; i < a.size_; ++i) {
      if (!(a[i] == b[i])) {
        return false;
      }
    }
    return true;
  }

  friend bool operator!=(const deque& a, const deque& b) { return !(a == b); }

 private:
  T* data_;
  size_type capacity_;
  size_type head_;
  size_type size_;

  size_type physical(size_type logical) const { return (head_ + logical) % capacity_; }

  void ensure_capacity() {
    if (size_ == capacity_) {
      grow(capacity_ == 0 ? 4 : capacity_ * 2);
    }
  }

  void grow(size_type new_cap) {
    T* new_data = static_cast<T*>(::operator new(new_cap * sizeof(T)));
    for (size_type i = 0; i < size_; ++i) {
      new (&new_data[i]) T(std::move(data_[physical(i)]));
      data_[physical(i)].~T();
    }
    ::operator delete(data_);
    data_ = new_data;
    capacity_ = new_cap;
    head_ = 0;
  }
};

// Deduction guide.
template <typename T>
deque(initializer_list<T>) -> deque<T>;

}  // namespace std

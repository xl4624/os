#pragma once

#include <algorithm.h>
#include <cstddef.h>
#include <initializer_list.h>
#include <iterator.h>
#include <new.h>
#include <type_traits.h>
#include <utility.h>

namespace std {

template <typename T>
class vector {
 public:
  using value_type = T;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using reference = value_type&;
  using const_reference = const value_type&;
  using iterator = value_type*;
  using const_iterator = const value_type*;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  vector() noexcept : data_(nullptr), size_(0), capacity_(0) {}

  explicit vector(size_type n) : data_(nullptr), size_(0), capacity_(0) {
    if (n == 0) {
      return;
    }
    data_ = allocate(n);
    if (data_ == nullptr) {
      return;
    }
    for (size_type i = 0; i < n; ++i) {
      new (data_ + i) value_type();
    }
    size_ = n;
    capacity_ = n;
  }

  vector(size_type n, const_reference value) : data_(nullptr), size_(0), capacity_(0) {
    if (n == 0) {
      return;
    }
    data_ = allocate(n);
    if (data_ == nullptr) {
      return;
    }
    for (size_type i = 0; i < n; ++i) {
      new (data_ + i) value_type(value);
    }
    size_ = n;
    capacity_ = n;
  }

  vector(initializer_list<value_type> il) : data_(nullptr), size_(0), capacity_(0) {
    if (il.size() == 0) {
      return;
    }
    data_ = allocate(il.size());
    if (data_ == nullptr) {
      return;
    }
    size_type i = 0;
    for (const_reference v : il) {
      new (data_ + i++) value_type(v);
    }
    size_ = il.size();
    capacity_ = il.size();
  }

  vector(const vector& other) : data_(nullptr), size_(0), capacity_(0) {
    if (other.size_ == 0) {
      return;
    }
    data_ = allocate(other.size_);
    if (data_ == nullptr) {
      return;
    }
    for (size_type i = 0; i < other.size_; ++i) {
      new (data_ + i) value_type(other.data_[i]);
    }
    size_ = other.size_;
    capacity_ = other.size_;
  }

  vector(vector&& other) noexcept
      : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
  }

  ~vector() {
    destroy_all();
    deallocate(data_);
  }

  // ===========================================================================
  // Assignment
  // ===========================================================================

  vector& operator=(const vector& other) {
    if (this == &other) {
      return *this;
    }
    destroy_all();
    if (other.size_ > capacity_) {
      deallocate(data_);
      data_ = allocate(other.size_);
      capacity_ = data_ != nullptr ? other.size_ : 0;
    }
    size_ = 0;
    if (data_ != nullptr) {
      for (size_type i = 0; i < other.size_; ++i) {
        new (data_ + i) value_type(other.data_[i]);
      }
      size_ = other.size_;
    }
    return *this;
  }

  vector& operator=(vector&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    destroy_all();
    deallocate(data_);
    data_ = other.data_;
    size_ = other.size_;
    capacity_ = other.capacity_;
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
    return *this;
  }

  vector& operator=(initializer_list<value_type> il) {
    assign(il);
    return *this;
  }

  void assign(size_type n, const_reference value) {
    destroy_all();
    if (n > capacity_) {
      deallocate(data_);
      data_ = allocate(n);
      capacity_ = data_ != nullptr ? n : 0;
    }
    size_ = 0;
    if (data_ != nullptr) {
      for (size_type i = 0; i < n; ++i) {
        new (data_ + i) value_type(value);
      }
      size_ = n;
    }
  }

  void assign(initializer_list<value_type> il) {
    destroy_all();
    if (il.size() > capacity_) {
      deallocate(data_);
      data_ = allocate(il.size());
      capacity_ = data_ != nullptr ? il.size() : 0;
    }
    size_ = 0;
    if (data_ != nullptr) {
      size_type i = 0;
      for (const_reference v : il) {
        new (data_ + i++) value_type(v);
      }
      size_ = il.size();
    }
  }

  // ===========================================================================
  // Element access
  // ===========================================================================

  reference operator[](size_type pos) { return data_[pos]; }
  const_reference operator[](size_type pos) const { return data_[pos]; }
  reference at(size_type pos) { return data_[pos]; }
  const_reference at(size_type pos) const { return data_[pos]; }
  reference front() { return data_[0]; }
  const_reference front() const { return data_[0]; }
  reference back() { return data_[size_ - 1]; }
  const_reference back() const { return data_[size_ - 1]; }
  pointer data() noexcept { return data_; }
  const_pointer data() const noexcept { return data_; }

  // ===========================================================================
  // Iterators
  // ===========================================================================

  iterator begin() noexcept { return data_; }
  const_iterator begin() const noexcept { return data_; }
  const_iterator cbegin() const noexcept { return data_; }
  iterator end() noexcept { return data_ + size_; }
  const_iterator end() const noexcept { return data_ + size_; }
  const_iterator cend() const noexcept { return data_ + size_; }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

  // ===========================================================================
  // Capacity
  // ===========================================================================

  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
  [[nodiscard]] size_type size() const noexcept { return size_; }
  [[nodiscard]] size_type capacity() const noexcept { return capacity_; }

  void reserve(size_type new_cap) {
    if (new_cap > capacity_) {
      grow(new_cap);
    }
  }

  void shrink_to_fit() {
    if (size_ >= capacity_) {
      return;
    }
    if (size_ == 0) {
      deallocate(data_);
      data_ = nullptr;
      capacity_ = 0;
      return;
    }
    pointer new_data = allocate(size_);
    if (new_data == nullptr) {
      return;
    }
    for (size_type i = 0; i < size_; ++i) {
      new (new_data + i) value_type(std::move(data_[i]));
      if constexpr (!is_trivially_destructible<value_type>::value) {
        data_[i].~value_type();
      }
    }
    deallocate(data_);
    data_ = new_data;
    capacity_ = size_;
  }

  // ===========================================================================
  // Modifiers
  // ===========================================================================

  void clear() noexcept {
    destroy_all();
    size_ = 0;
  }

  void push_back(const_reference value) {
    if (size_ == capacity_) {
      if (!grow(capacity_ == 0 ? 1 : capacity_ * kGrowthFactor)) {
        return;
      }
    }
    new (data_ + size_) value_type(value);
    ++size_;
  }

  void push_back(value_type&& value) {
    if (size_ == capacity_) {
      if (!grow(capacity_ == 0 ? 1 : capacity_ * kGrowthFactor)) {
        return;
      }
    }
    new (data_ + size_) value_type(std::move(value));
    ++size_;
  }

  template <typename... Args>
  reference emplace_back(Args&&... args) {
    if (size_ == capacity_) {
      grow(capacity_ == 0 ? 1 : capacity_ * kGrowthFactor);
    }
    new (data_ + size_) value_type(std::forward<Args>(args)...);
    return data_[size_++];
  }

  void pop_back() {
    --size_;
    data_[size_].~value_type();
  }

  void resize(size_type n) {
    if (n < size_) {
      for (size_type i = n; i < size_; ++i) {
        data_[i].~value_type();
      }
      size_ = n;
    } else if (n > size_) {
      if (n > capacity_ && !grow(n)) {
        return;
      }
      for (size_type i = size_; i < n; ++i) {
        new (data_ + i) value_type();
      }
      size_ = n;
    }
  }

  void resize(size_type n, const_reference value) {
    if (n < size_) {
      for (size_type i = n; i < size_; ++i) {
        data_[i].~value_type();
      }
      size_ = n;
    } else if (n > size_) {
      if (n > capacity_ && !grow(n)) {
        return;
      }
      for (size_type i = size_; i < n; ++i) {
        new (data_ + i) value_type(value);
      }
      size_ = n;
    }
  }

  // Insert a single element before pos. Returns iterator to the inserted element.
  iterator insert(const_iterator pos, const_reference value) {
    auto idx = static_cast<size_type>(pos - cbegin());
    if (size_ == capacity_) {
      if (!grow(capacity_ == 0 ? 1 : capacity_ * kGrowthFactor)) {
        return begin() + static_cast<difference_type>(idx);
      }
    }
    // Shift elements right to make room.
    if (idx < size_) {
      new (data_ + size_) value_type(std::move(data_[size_ - 1]));
      for (size_type i = size_ - 1; i > idx; --i) {
        data_[i] = std::move(data_[i - 1]);
      }
      data_[idx].~value_type();
    }
    new (data_ + idx) value_type(value);
    ++size_;
    return data_ + idx;
  }

  iterator insert(const_iterator pos, value_type&& value) {
    auto idx = static_cast<size_type>(pos - cbegin());
    if (size_ == capacity_) {
      if (!grow(capacity_ == 0 ? 1 : capacity_ * kGrowthFactor)) {
        return begin() + static_cast<difference_type>(idx);
      }
    }
    if (idx < size_) {
      new (data_ + size_) value_type(std::move(data_[size_ - 1]));
      for (size_type i = size_ - 1; i > idx; --i) {
        data_[i] = std::move(data_[i - 1]);
      }
      data_[idx].~value_type();
    }
    new (data_ + idx) value_type(std::move(value));
    ++size_;
    return data_ + idx;
  }

  // Erase the element at pos. Returns iterator to the element after the erased one.
  iterator erase(const_iterator pos) {
    auto it = const_cast<iterator>(pos);
    it->~value_type();
    iterator last = end() - 1;
    for (iterator p = it; p != last; ++p) {
      new (p) value_type(std::move(*(p + 1)));
      (p + 1)->~value_type();
    }
    --size_;
    return it;
  }

  // Erase elements in [first, last). Returns iterator to the element after the last erased one.
  iterator erase(const_iterator first, const_iterator last) {
    auto f = const_cast<iterator>(first);
    auto l = const_cast<iterator>(last);
    auto count = static_cast<size_type>(l - f);
    for (auto p = f; p != l; ++p) {
      p->~value_type();
    }
    // Move the tail left to fill the gap.
    iterator dst = f;
    for (iterator src = l; src != end(); ++src, ++dst) {
      new (dst) value_type(std::move(*src));
      src->~value_type();
    }
    size_ -= count;
    return f;
  }

  void swap(vector& other) noexcept {
    pointer tmp_data = data_;
    size_type tmp_size = size_;
    size_type tmp_cap = capacity_;
    data_ = other.data_;
    size_ = other.size_;
    capacity_ = other.capacity_;
    other.data_ = tmp_data;
    other.size_ = tmp_size;
    other.capacity_ = tmp_cap;
  }

 private:
  static constexpr size_type kGrowthFactor = 2;

  pointer data_;
  size_type size_;
  size_type capacity_;

  static pointer allocate(size_type n) {
    return static_cast<pointer>(::operator new(n * sizeof(value_type)));
  }

  static void deallocate(pointer ptr) { ::operator delete(ptr); }

  void destroy_all() noexcept {
    if constexpr (!is_trivially_destructible<value_type>::value) {
      for (size_type i = 0; i < size_; ++i) {
        data_[i].~value_type();
      }
    }
  }

  // Reallocate to new_cap, moving existing elements. Returns false on failure.
  bool grow(size_type new_cap) {
    pointer new_data = allocate(new_cap);
    if (new_data == nullptr) {
      return false;
    }
    for (size_type i = 0; i < size_; ++i) {
      new (new_data + i) value_type(std::move(data_[i]));
      if constexpr (!is_trivially_destructible<value_type>::value) {
        data_[i].~value_type();
      }
    }
    deallocate(data_);
    data_ = new_data;
    capacity_ = new_cap;
    return true;
  }
};

// ===========================================================================
// Non-member comparison operators
// ===========================================================================

template <typename T>
bool operator==(const vector<T>& a, const vector<T>& b) {
  if (a.size() != b.size()) {
    return false;
  }
  return std::equal(a.begin(), a.end(), b.begin());
}

template <typename T>
bool operator!=(const vector<T>& a, const vector<T>& b) {
  return !(a == b);
}

template <typename T>
bool operator<(const vector<T>& a, const vector<T>& b) {
  return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

template <typename T>
bool operator>(const vector<T>& a, const vector<T>& b) {
  return b < a;
}

template <typename T>
bool operator<=(const vector<T>& a, const vector<T>& b) {
  return !(a > b);
}

template <typename T>
bool operator>=(const vector<T>& a, const vector<T>& b) {
  return !(a < b);
}

// Deduction guide: vector{1, 2, 3} -> vector<int>
template <typename T>
vector(std::initializer_list<T>) -> vector<T>;

}  // namespace std

#pragma once

#include <algorithm.h>
#include <cstddef.h>
#include <cstdlib.h>
#include <cstring.h>
#include <iterator.h>

namespace std {

class string {
 public:
  using value_type = char;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using reference = char&;
  using const_reference = const char&;
  using pointer = char*;
  using const_pointer = const char*;
  using iterator = char*;
  using const_iterator = const char*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  static const size_type npos = static_cast<size_type>(-1);

  // =========================================================================
  // Constructor
  // =========================================================================

  string() noexcept : data_(nullptr), size_(0), capacity_(0) {}

  string(const char* s) : string() {
    if (s != nullptr) {
      const size_type n = strlen(s);
      if (n > 0) {
        assign(s, n);
      }
    }
  }

  string(const char* s, size_type n) : string() { assign(s, n); }

  string(size_type n, char c) : string() {
    if (n == 0) {
      return;
    }
    if (!grow(n)) {
      return;
    }
    memset(data_, static_cast<int>(c), n);
    data_[n] = '\0';
    size_ = n;
  }

  string(const string& other) : string() { assign(other.data(), other.size_); }

  string(string&& other) noexcept
      : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
  }

  ~string() { free(data_); }

  // =========================================================================
  // Assignment
  // =========================================================================

  string& operator=(const string& other) {
    if (this != &other) {
      assign(other.data(), other.size_);
    }
    return *this;
  }

  string& operator=(string&& other) noexcept {
    if (this != &other) {
      free(data_);
      data_ = other.data_;
      size_ = other.size_;
      capacity_ = other.capacity_;
      other.data_ = nullptr;
      other.size_ = 0;
      other.capacity_ = 0;
    }
    return *this;
  }

  string& operator=(const char* s) {
    if (s == nullptr) {
      clear();
    } else {
      assign(s, strlen(s));
    }
    return *this;
  }

  string& assign(const char* s, size_type n) {
    if (n == 0) {
      clear();
      return *this;
    }
    if (!grow(n)) {
      return *this;
    }
    memmove(data_, s, n);
    data_[n] = '\0';
    size_ = n;
    return *this;
  }

  string& assign(const string& other) { return assign(other.data(), other.size_); }

  string& assign(size_type n, char c) {
    if (n == 0) {
      clear();
      return *this;
    }
    if (!grow(n)) {
      return *this;
    }
    memset(data_, static_cast<int>(c), n);
    data_[n] = '\0';
    size_ = n;
    return *this;
  }

  // =========================================================================
  // Capacity
  // =========================================================================

  [[nodiscard]] size_type size() const noexcept { return size_; }
  [[nodiscard]] size_type length() const noexcept { return size_; }
  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
  [[nodiscard]] size_type capacity() const noexcept { return capacity_; }

  void reserve(size_type new_cap) {
    if (new_cap <= capacity_) {
      return;
    }
    char* p = static_cast<char*>(realloc(data_, new_cap + 1));
    if (p == nullptr) {
      return;
    }
    data_ = p;
    capacity_ = new_cap;
  }

  void resize(size_type n, char c = '\0') {
    if (n < size_) {
      data_[n] = '\0';
      size_ = n;
    } else if (n > size_) {
      if (!grow(n)) {
        return;
      }
      memset(data_ + size_, static_cast<int>(c), n - size_);
      data_[n] = '\0';
      size_ = n;
    }
  }

  void shrink_to_fit() {
    if (capacity_ == size_) {
      return;
    }
    if (size_ == 0) {
      free(data_);
      data_ = nullptr;
      capacity_ = 0;
      return;
    }
    char* p = static_cast<char*>(realloc(data_, size_ + 1));
    if (p != nullptr) {
      data_ = p;
      capacity_ = size_;
    }
  }

  void clear() noexcept {
    if (data_ != nullptr) {
      data_[0] = '\0';
    }
    size_ = 0;
  }

  // =========================================================================
  // Element access
  // =========================================================================

  [[nodiscard]] reference operator[](size_type i) { return data_[i]; }
  [[nodiscard]] const_reference operator[](size_type i) const { return data_[i]; }

  [[nodiscard]] reference front() { return data_[0]; }
  [[nodiscard]] const_reference front() const { return data_[0]; }
  [[nodiscard]] reference back() { return data_[size_ - 1]; }
  [[nodiscard]] const_reference back() const { return data_[size_ - 1]; }

  [[nodiscard]] pointer data() noexcept { return data_ != nullptr ? data_ : const_cast<char*>(""); }
  [[nodiscard]] const_pointer data() const noexcept { return data_ != nullptr ? data_ : ""; }
  [[nodiscard]] const_pointer c_str() const noexcept { return data(); }

  // =========================================================================
  // Iterators
  // =========================================================================

  iterator begin() noexcept { return data(); }
  iterator end() noexcept { return data() + size_; }
  [[nodiscard]] const_iterator begin() const noexcept { return data(); }
  [[nodiscard]] const_iterator end() const noexcept { return data() + size_; }
  [[nodiscard]] const_iterator cbegin() const noexcept { return data(); }
  [[nodiscard]] const_iterator cend() const noexcept { return data() + size_; }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  [[nodiscard]] const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  [[nodiscard]] const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  [[nodiscard]] const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
  [[nodiscard]] const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

  // =========================================================================
  // Modifiers
  // =========================================================================

  string& push_back(char c) {
    if (!grow(size_ + 1)) {
      return *this;
    }
    data_[size_++] = c;
    data_[size_] = '\0';
    return *this;
  }

  void pop_back() {
    if (size_ > 0) {
      data_[--size_] = '\0';
    }
  }

  string& append(const char* s, size_type n) {
    if (n == 0 || s == nullptr) {
      return *this;
    }
    const size_type new_size = size_ + n;
    if (!grow(new_size)) {
      return *this;
    }
    memmove(data_ + size_, s, n);
    data_[new_size] = '\0';
    size_ = new_size;
    return *this;
  }

  string& append(const char* s) {
    if (s != nullptr) {
      append(s, strlen(s));
    }
    return *this;
  }

  string& append(const string& other) { return append(other.data(), other.size_); }

  string& append(size_type n, char c) {
    if (n == 0) {
      return *this;
    }
    const size_type new_size = size_ + n;
    if (!grow(new_size)) {
      return *this;
    }
    memset(data_ + size_, static_cast<int>(c), n);
    data_[new_size] = '\0';
    size_ = new_size;
    return *this;
  }

  string& operator+=(const string& other) { return append(other); }
  string& operator+=(const char* s) { return append(s); }
  string& operator+=(char c) { return push_back(c); }

  string& insert(size_type pos, const char* s) {
    if (pos > size_ || s == nullptr) {
      return *this;
    }
    const size_type n = strlen(s);
    if (n == 0) {
      return *this;
    }
    const size_type new_size = size_ + n;
    if (!grow(new_size)) {
      return *this;
    }
    memmove(data_ + pos + n, data_ + pos, size_ - pos + 1);
    memmove(data_ + pos, s, n);
    size_ = new_size;
    return *this;
  }

  string& erase(size_type pos = 0, size_type len = npos) {
    if (pos >= size_) {
      return *this;
    }
    const size_type actual = std::min(len, size_ - pos);
    memmove(data_ + pos, data_ + pos + actual, size_ - pos - actual + 1);
    size_ -= actual;
    return *this;
  }

  // =========================================================================
  // Find
  // =========================================================================

  [[nodiscard]] size_type find(char c, size_type pos = 0) const noexcept {
    for (size_type i = pos; i < size_; ++i) {
      if (data_[i] == c) {
        return i;
      }
    }
    return npos;
  }

  [[nodiscard]] size_type find(const char* s, size_type pos = 0) const noexcept {
    if (s == nullptr || s[0] == '\0') {
      return pos <= size_ ? pos : npos;
    }
    const size_type n = strlen(s);
    if (n > size_) {
      return npos;
    }
    for (size_type i = pos; i + n <= size_; ++i) {
      if (memcmp(data_ + i, s, n) == 0) {
        return i;
      }
    }
    return npos;
  }

  [[nodiscard]] size_type find(const string& other, size_type pos = 0) const noexcept {
    return find(other.data(), pos);
  }

  [[nodiscard]] size_type rfind(char c, size_type pos = npos) const noexcept {
    if (size_ == 0) {
      return npos;
    }
    size_type i = (pos >= size_) ? (size_ - 1) : pos;
    do {
      if (data_[i] == c) {
        return i;
      }
    } while (i-- > 0);
    return npos;
  }

  [[nodiscard]] size_type rfind(const char* s, size_type pos = npos) const noexcept {
    if (s == nullptr) {
      return npos;
    }
    const size_type n = strlen(s);
    if (n == 0) {
      return std::min(pos, size_);
    }
    if (n > size_) {
      return npos;
    }
    size_type i = std::min(pos, size_ - n);
    do {
      if (memcmp(data_ + i, s, n) == 0) {
        return i;
      }
    } while (i-- > 0);
    return npos;
  }

  // =========================================================================
  // Substr
  // =========================================================================

  [[nodiscard]] string substr(size_type pos = 0, size_type len = npos) const {
    if (pos > size_) {
      return string{};
    }
    const size_type actual = std::min(len, size_ - pos);
    return string(data_ + pos, actual);
  }

  // =========================================================================
  // Compare
  // =========================================================================

  [[nodiscard]] int compare(const string& other) const noexcept {
    const size_type min_len = std::min(size_, other.size_);
    const int r = memcmp(data(), other.data(), min_len);
    if (r != 0) {
      return r;
    }
    if (size_ < other.size_) {
      return -1;
    }
    if (size_ > other.size_) {
      return 1;
    }
    return 0;
  }

  [[nodiscard]] int compare(const char* s) const noexcept {
    if (s == nullptr) {
      return size_ == 0 ? 0 : 1;
    }
    return strcmp(data(), s);
  }

  // =========================================================================
  // Relational operators (free functions defined below)
  // =========================================================================

 private:
  char* data_;
  size_type size_;
  size_type capacity_;

  // Ensure capacity_ >= needed. Does NOT update size_. Returns false on OOM.
  bool grow(size_type needed) {
    if (needed <= capacity_) {
      return true;
    }
    // 2x growth strategy, minimum 15 chars.
    size_type new_cap = capacity_ == 0 ? 15 : capacity_ * 2;
    if (new_cap < needed) {
      new_cap = needed;
    }
    char* p = static_cast<char*>(realloc(data_, new_cap + 1));
    if (p == nullptr) {
      return false;
    }
    data_ = p;
    capacity_ = new_cap;
    return true;
  }
};

// ===========================================================================
// Free-function relational operators
// ===========================================================================

inline bool operator==(const string& a, const string& b) noexcept {
  return a.size() == b.size() && memcmp(a.data(), b.data(), a.size()) == 0;
}
inline bool operator!=(const string& a, const string& b) noexcept { return !(a == b); }
inline bool operator<(const string& a, const string& b) noexcept { return a.compare(b) < 0; }
inline bool operator<=(const string& a, const string& b) noexcept { return a.compare(b) <= 0; }
inline bool operator>(const string& a, const string& b) noexcept { return a.compare(b) > 0; }
inline bool operator>=(const string& a, const string& b) noexcept { return a.compare(b) >= 0; }

inline bool operator==(const string& a, const char* b) noexcept { return a.compare(b) == 0; }
inline bool operator==(const char* a, const string& b) noexcept { return b == a; }
inline bool operator!=(const string& a, const char* b) noexcept { return !(a == b); }
inline bool operator!=(const char* a, const string& b) noexcept { return !(b == a); }

inline string operator+(string a, const string& b) {
  a.append(b);
  return a;
}
inline string operator+(string a, const char* b) {
  a.append(b);
  return a;
}
inline string operator+(const char* a, const string& b) {
  string s(a);
  s.append(b);
  return s;
}
inline string operator+(string a, char c) {
  a.push_back(c);
  return a;
}

// ===========================================================================
// to_string helpers
// ===========================================================================

namespace detail {
inline string uint_to_string(unsigned long long v) {
  if (v == 0) {
    return string("0");
  }
  char buf[24];
  int i = 23;
  buf[i] = '\0';
  while (v > 0 && i > 0) {
    buf[--i] = static_cast<char>('0' + v % 10);
    v /= 10;
  }
  return string(buf + i);
}
}  // namespace detail

inline string to_string(int v) {
  if (v < 0) {
    return string("-") + detail::uint_to_string(static_cast<unsigned long long>(-v));
  }
  return detail::uint_to_string(static_cast<unsigned long long>(v));
}
inline string to_string(unsigned int v) {
  return detail::uint_to_string(static_cast<unsigned long long>(v));
}
inline string to_string(long v) {
  if (v < 0) {
    return string("-") + detail::uint_to_string(static_cast<unsigned long long>(-v));
  }
  return detail::uint_to_string(static_cast<unsigned long long>(v));
}
inline string to_string(unsigned long v) {
  return detail::uint_to_string(static_cast<unsigned long long>(v));
}
inline string to_string(long long v) {
  if (v < 0) {
    return string("-") + detail::uint_to_string(static_cast<unsigned long long>(-v));
  }
  return detail::uint_to_string(static_cast<unsigned long long>(v));
}
inline string to_string(unsigned long long v) { return detail::uint_to_string(v); }

}  // namespace std

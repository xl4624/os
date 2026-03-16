#pragma once

#include <cstddef.h>
#include <type_traits.h>

namespace std {

// ===========================================================================
// Iterator category tags
// ===========================================================================

struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : input_iterator_tag {};
struct bidirectional_iterator_tag : forward_iterator_tag {};
struct random_access_iterator_tag : bidirectional_iterator_tag {};

// ===========================================================================
// iterator_traits
// ===========================================================================

template <typename Iter>
struct iterator_traits {
  using iterator_category = typename Iter::iterator_category;
  using value_type        = typename Iter::value_type;
  using difference_type   = typename Iter::difference_type;
  using pointer           = typename Iter::pointer;
  using reference         = typename Iter::reference;
};

template <typename T>
struct iterator_traits<T*> {
  using iterator_category = random_access_iterator_tag;
  using value_type        = remove_cv_t<T>;
  using difference_type   = ptrdiff_t;
  using pointer           = T*;
  using reference         = T&;
};

template <typename T>
struct iterator_traits<const T*> {
  using iterator_category = random_access_iterator_tag;
  using value_type        = T;
  using difference_type   = ptrdiff_t;
  using pointer           = const T*;
  using reference         = const T&;
};

// ===========================================================================
// reverse_iterator
// ===========================================================================

template <typename Iter>
class reverse_iterator {
 public:
  using iterator_type     = Iter;
  using iterator_category = typename iterator_traits<Iter>::iterator_category;
  using value_type        = typename iterator_traits<Iter>::value_type;
  using difference_type   = typename iterator_traits<Iter>::difference_type;
  using pointer           = typename iterator_traits<Iter>::pointer;
  using reference         = typename iterator_traits<Iter>::reference;

  constexpr reverse_iterator() : current_() {}
  constexpr explicit reverse_iterator(Iter it) : current_(it) {}

  template <typename U>
  constexpr reverse_iterator(const reverse_iterator<U>& other) : current_(other.base()) {}

  constexpr Iter base() const { return current_; }

  constexpr reference operator*() const {
    Iter tmp = current_;
    return *--tmp;
  }

  constexpr pointer operator->() const {
    Iter tmp = current_;
    --tmp;
    return &(*tmp);
  }

  constexpr reference operator[](difference_type n) const { return *(*this + n); }

  // Increment/decrement move backwards through the sequence.
  constexpr reverse_iterator& operator++() {
    --current_;
    return *this;
  }
  constexpr reverse_iterator operator++(int) {
    reverse_iterator tmp = *this;
    --current_;
    return tmp;
  }
  constexpr reverse_iterator& operator--() {
    ++current_;
    return *this;
  }
  constexpr reverse_iterator operator--(int) {
    reverse_iterator tmp = *this;
    ++current_;
    return tmp;
  }

  constexpr reverse_iterator operator+(difference_type n) const {
    return reverse_iterator(current_ - n);
  }
  constexpr reverse_iterator operator-(difference_type n) const {
    return reverse_iterator(current_ + n);
  }
  constexpr reverse_iterator& operator+=(difference_type n) {
    current_ -= n;
    return *this;
  }
  constexpr reverse_iterator& operator-=(difference_type n) {
    current_ += n;
    return *this;
  }

 private:
  Iter current_;
};

// Non-member comparison operators.
template <typename Iter1, typename Iter2>
constexpr bool operator==(const reverse_iterator<Iter1>& a, const reverse_iterator<Iter2>& b) {
  return a.base() == b.base();
}
template <typename Iter1, typename Iter2>
constexpr bool operator!=(const reverse_iterator<Iter1>& a, const reverse_iterator<Iter2>& b) {
  return a.base() != b.base();
}
template <typename Iter1, typename Iter2>
constexpr bool operator<(const reverse_iterator<Iter1>& a, const reverse_iterator<Iter2>& b) {
  return a.base() > b.base();
}
template <typename Iter1, typename Iter2>
constexpr bool operator<=(const reverse_iterator<Iter1>& a, const reverse_iterator<Iter2>& b) {
  return a.base() >= b.base();
}
template <typename Iter1, typename Iter2>
constexpr bool operator>(const reverse_iterator<Iter1>& a, const reverse_iterator<Iter2>& b) {
  return a.base() < b.base();
}
template <typename Iter1, typename Iter2>
constexpr bool operator>=(const reverse_iterator<Iter1>& a, const reverse_iterator<Iter2>& b) {
  return a.base() <= b.base();
}

template <typename Iter>
constexpr reverse_iterator<Iter> operator+(
    typename reverse_iterator<Iter>::difference_type n, const reverse_iterator<Iter>& it) {
  return it + n;
}

template <typename Iter1, typename Iter2>
constexpr auto operator-(const reverse_iterator<Iter1>& a,
                         const reverse_iterator<Iter2>& b) -> decltype(b.base() - a.base()) {
  return b.base() - a.base();
}

// ===========================================================================
// begin() / end() free functions
// ===========================================================================

// C-array overloads.
template <typename T, size_t N>
constexpr T* begin(T (&arr)[N]) noexcept {
  return arr;
}

template <typename T, size_t N>
constexpr T* end(T (&arr)[N]) noexcept {
  return arr + N;
}

// Container overloads (forward to member begin/end).
template <typename C>
constexpr auto begin(C& c) -> decltype(c.begin()) {
  return c.begin();
}

template <typename C>
constexpr auto begin(const C& c) -> decltype(c.begin()) {
  return c.begin();
}

template <typename C>
constexpr auto end(C& c) -> decltype(c.end()) {
  return c.end();
}

template <typename C>
constexpr auto end(const C& c) -> decltype(c.end()) {
  return c.end();
}

// cbegin/cend.
template <typename C>
constexpr auto cbegin(const C& c) -> decltype(begin(c)) {
  return begin(c);
}

template <typename C>
constexpr auto cend(const C& c) -> decltype(end(c)) {
  return end(c);
}

// rbegin/rend for containers.
template <typename C>
constexpr auto rbegin(C& c) -> decltype(c.rbegin()) {
  return c.rbegin();
}

template <typename C>
constexpr auto rbegin(const C& c) -> decltype(c.rbegin()) {
  return c.rbegin();
}

template <typename C>
constexpr auto rend(C& c) -> decltype(c.rend()) {
  return c.rend();
}

template <typename C>
constexpr auto rend(const C& c) -> decltype(c.rend()) {
  return c.rend();
}

// rbegin/rend for C arrays.
template <typename T, size_t N>
constexpr reverse_iterator<T*> rbegin(T (&arr)[N]) noexcept {
  return reverse_iterator<T*>(arr + N);
}

template <typename T, size_t N>
constexpr reverse_iterator<T*> rend(T (&arr)[N]) noexcept {
  return reverse_iterator<T*>(arr);
}

// crbegin/crend.
template <typename C>
constexpr auto crbegin(const C& c) -> decltype(rbegin(c)) {
  return rbegin(c);
}

template <typename C>
constexpr auto crend(const C& c) -> decltype(rend(c)) {
  return rend(c);
}

}  // namespace std

#pragma once

#include <algorithm.h>
#include <cstddef.h>
#include <initializer_list.h>
#include <iterator.h>
#include <new.h>
#include <utility.h>

namespace std {

template <typename T>
class list {
  struct Node {
    T value;
    Node* prev;
    Node* next;

    template <typename... Args>
    explicit Node(Args&&... args)
        : value(std::forward<Args>(args)...), prev(nullptr), next(nullptr) {}
  };

 public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

  // Bidirectional iterator.
  class iterator {
   public:
    using iterator_category = bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    iterator() : node_(nullptr) {}
    explicit iterator(Node* n) : node_(n) {}

    reference operator*() const { return node_->value; }
    pointer operator->() const { return &node_->value; }

    iterator& operator++() {
      node_ = node_->next;
      return *this;
    }

    iterator operator++(int) {
      iterator tmp = *this;
      node_ = node_->next;
      return tmp;
    }

    iterator& operator--() {
      node_ = node_->prev;
      return *this;
    }

    iterator operator--(int) {
      iterator tmp = *this;
      node_ = node_->prev;
      return tmp;
    }

    bool operator==(const iterator& other) const { return node_ == other.node_; }
    bool operator!=(const iterator& other) const { return node_ != other.node_; }

   private:
    friend class list;
    Node* node_;
  };

  class const_iterator {
   public:
    using iterator_category = bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;

    const_iterator() : node_(nullptr) {}
    explicit const_iterator(const Node* n) : node_(n) {}
    const_iterator(iterator it) : node_(it.node_) {}  // NOLINT: intentional implicit

    reference operator*() const { return node_->value; }
    pointer operator->() const { return &node_->value; }

    const_iterator& operator++() {
      node_ = node_->next;
      return *this;
    }

    const_iterator operator++(int) {
      const_iterator tmp = *this;
      node_ = node_->next;
      return tmp;
    }

    const_iterator& operator--() {
      node_ = node_->prev;
      return *this;
    }

    const_iterator operator--(int) {
      const_iterator tmp = *this;
      node_ = node_->prev;
      return tmp;
    }

    bool operator==(const const_iterator& other) const { return node_ == other.node_; }
    bool operator!=(const const_iterator& other) const { return node_ != other.node_; }

   private:
    friend class list;
    const Node* node_;
  };

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // =========================================================================
  // Construction / destruction
  // =========================================================================

  list() { init_sentinel(); }

  list(size_type n, const_reference value) {
    init_sentinel();
    for (size_type i = 0; i < n; ++i) {
      push_back(value);
    }
  }

  list(initializer_list<T> il) {
    init_sentinel();
    for (const auto& v : il) {
      push_back(v);
    }
  }

  list(const list& other) {
    init_sentinel();
    for (const auto& v : other) {
      push_back(v);
    }
  }

  list(list&& other) noexcept {
    init_sentinel();
    splice(end(), other);
  }

  ~list() {
    clear();
    // Sentinel is embedded, no need to free.
  }

  // =========================================================================
  // Assignment
  // =========================================================================

  list& operator=(const list& other) {
    if (this != &other) {
      list tmp(other);
      swap(tmp);
    }
    return *this;
  }

  list& operator=(list&& other) noexcept {
    if (this != &other) {
      clear();
      splice(end(), other);
    }
    return *this;
  }

  list& operator=(initializer_list<T> il) {
    list tmp(il);
    swap(tmp);
    return *this;
  }

  // =========================================================================
  // Element access
  // =========================================================================

  reference front() { return sentinel_.next->value; }
  const_reference front() const { return sentinel_.next->value; }
  reference back() { return sentinel_.prev->value; }
  const_reference back() const { return sentinel_.prev->value; }

  // =========================================================================
  // Iterators
  // =========================================================================

  iterator begin() noexcept { return iterator(sentinel_.next); }
  const_iterator begin() const noexcept { return const_iterator(sentinel_.next); }
  const_iterator cbegin() const noexcept { return const_iterator(sentinel_.next); }
  iterator end() noexcept { return iterator(sentinel_ptr()); }
  const_iterator end() const noexcept { return const_iterator(sentinel_ptr()); }
  const_iterator cend() const noexcept { return const_iterator(sentinel_ptr()); }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

  // =========================================================================
  // Capacity
  // =========================================================================

  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
  [[nodiscard]] size_type size() const noexcept { return size_; }

  // =========================================================================
  // Modifiers
  // =========================================================================

  void clear() noexcept {
    Node* cur = sentinel_.next;
    while (cur != sentinel_ptr()) {
      Node* next = cur->next;
      cur->~Node();
      ::operator delete(cur);
      cur = next;
    }
    sentinel_.next = sentinel_ptr();
    sentinel_.prev = sentinel_ptr();
    size_ = 0;
  }

  void push_back(const_reference value) { insert(end(), value); }
  void push_back(T&& value) { insert(end(), std::move(value)); }
  void push_front(const_reference value) { insert(begin(), value); }
  void push_front(T&& value) { insert(begin(), std::move(value)); }

  template <typename... Args>
  reference emplace_back(Args&&... args) {
    return *emplace(end(), std::forward<Args>(args)...);
  }

  template <typename... Args>
  reference emplace_front(Args&&... args) {
    return *emplace(begin(), std::forward<Args>(args)...);
  }

  void pop_back() { erase(iterator(sentinel_.prev)); }
  void pop_front() { erase(begin()); }

  iterator insert(const_iterator pos, const_reference value) {
    Node* node = new Node(value);
    return link_before(pos, node);
  }

  iterator insert(const_iterator pos, T&& value) {
    Node* node = new Node(std::move(value));
    return link_before(pos, node);
  }

  template <typename... Args>
  iterator emplace(const_iterator pos, Args&&... args) {
    Node* node = new Node(std::forward<Args>(args)...);
    return link_before(pos, node);
  }

  iterator erase(const_iterator pos) {
    Node* node = const_cast<Node*>(pos.node_);
    Node* next = node->next;
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->~Node();
    ::operator delete(node);
    --size_;
    return iterator(next);
  }

  iterator erase(const_iterator first, const_iterator last) {
    while (first != last) {
      first = erase(first);
    }
    return iterator(const_cast<Node*>(last.node_));
  }

  // Splice: transfer all elements from other before pos.
  void splice(const_iterator pos, list& other) {
    if (other.empty()) {
      return;
    }
    Node* first = other.sentinel_.next;
    Node* last = other.sentinel_.prev;
    // Unlink from other.
    other.sentinel_.next = other.sentinel_ptr();
    other.sentinel_.prev = other.sentinel_ptr();
    // Link into this list before pos.
    Node* p = const_cast<Node*>(pos.node_);
    first->prev = p->prev;
    last->next = p;
    p->prev->next = first;
    p->prev = last;
    size_ += other.size_;
    other.size_ = 0;
  }

  void swap(list& other) noexcept {
    // For sentinel-based lists, we need to fix up pointers.
    list tmp;
    tmp.splice(tmp.end(), other);
    other.splice(other.end(), *this);
    splice(end(), tmp);
  }

  // =========================================================================
  // Comparison
  // =========================================================================

  friend bool operator==(const list& a, const list& b) {
    if (a.size_ != b.size_) {
      return false;
    }
    return std::equal(a.begin(), a.end(), b.begin());
  }

  friend bool operator!=(const list& a, const list& b) { return !(a == b); }

 private:
  // Sentinel node: we store prev/next pointers but no value.
  // Reinterpret the sentinel storage as a Node* for linking.
  struct SentinelStorage {
    alignas(Node) char padding[sizeof(T)];  // Placeholder for value field.
    Node* prev;
    Node* next;
  };

  SentinelStorage sentinel_;
  size_type size_ = 0;

  Node* sentinel_ptr() { return reinterpret_cast<Node*>(&sentinel_); }
  const Node* sentinel_ptr() const { return reinterpret_cast<const Node*>(&sentinel_); }

  void init_sentinel() {
    sentinel_.prev = sentinel_ptr();
    sentinel_.next = sentinel_ptr();
    size_ = 0;
  }

  iterator link_before(const_iterator pos, Node* node) {
    Node* p = const_cast<Node*>(pos.node_);
    node->next = p;
    node->prev = p->prev;
    p->prev->next = node;
    p->prev = node;
    ++size_;
    return iterator(node);
  }
};

// Deduction guide.
template <typename T>
list(initializer_list<T>) -> list<T>;

}  // namespace std

#pragma once

#include <cstddef.h>
#include <initializer_list.h>
#include <new.h>
#include <utility.h>
#include <vector.h>

namespace std {

// Default hash: uses __builtin for integers, pointer-based for pointers.
template <typename Key>
struct hash;

template <>
struct hash<int> {
  size_t operator()(int v) const noexcept { return static_cast<size_t>(v); }
};

template <>
struct hash<unsigned int> {
  size_t operator()(unsigned int v) const noexcept { return static_cast<size_t>(v); }
};

template <>
struct hash<long> {
  size_t operator()(long v) const noexcept { return static_cast<size_t>(v); }
};

template <>
struct hash<unsigned long> {
  size_t operator()(unsigned long v) const noexcept { return static_cast<size_t>(v); }
};

template <>
struct hash<long long> {
  size_t operator()(long long v) const noexcept { return static_cast<size_t>(v); }
};

template <>
struct hash<unsigned long long> {
  size_t operator()(unsigned long long v) const noexcept { return static_cast<size_t>(v); }
};

template <>
struct hash<char> {
  size_t operator()(char v) const noexcept { return static_cast<size_t>(v); }
};

template <>
struct hash<signed char> {
  size_t operator()(signed char v) const noexcept { return static_cast<size_t>(v); }
};

template <>
struct hash<unsigned char> {
  size_t operator()(unsigned char v) const noexcept { return static_cast<size_t>(v); }
};

template <>
struct hash<short> {
  size_t operator()(short v) const noexcept { return static_cast<size_t>(v); }
};

template <>
struct hash<unsigned short> {
  size_t operator()(unsigned short v) const noexcept { return static_cast<size_t>(v); }
};

template <typename T>
struct hash<T*> {
  size_t operator()(T* p) const noexcept { return reinterpret_cast<size_t>(p); }
};

// Equal-to functor.
template <typename T>
struct equal_to {
  constexpr bool operator()(const T& a, const T& b) const { return a == b; }
};

// Separate-chaining hash map.
template <typename Key, typename Value, typename Hash = hash<Key>,
          typename KeyEqual = equal_to<Key>>
class unordered_map {
 public:
  using key_type = Key;
  using mapped_type = Value;
  using value_type = pair<const Key, Value>;
  using size_type = size_t;
  using hasher = Hash;
  using key_equal = KeyEqual;

 private:
  struct Node {
    value_type kv;
    Node* next;

    template <typename K, typename V>
    Node(K&& k, V&& v) : kv(std::forward<K>(k), std::forward<V>(v)), next(nullptr) {}

    explicit Node(const value_type& p) : kv(p), next(nullptr) {}

    template <typename K>
    explicit Node(K&& k) : kv(std::forward<K>(k), Value()), next(nullptr) {}
  };

 public:
  class iterator {
   public:
    using value_type = pair<const Key, Value>;
    using reference = value_type&;
    using pointer = value_type*;
    using difference_type = ptrdiff_t;

    iterator() : map_(nullptr), bucket_(0), node_(nullptr) {}
    iterator(const unordered_map* m, size_type b, Node* n) : map_(m), bucket_(b), node_(n) {}

    reference operator*() const { return node_->kv; }
    pointer operator->() const { return &node_->kv; }

    iterator& operator++() {
      node_ = node_->next;
      if (node_ == nullptr) {
        ++bucket_;
        advance_to_valid();
      }
      return *this;
    }

    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator==(const iterator& other) const { return node_ == other.node_; }
    bool operator!=(const iterator& other) const { return node_ != other.node_; }

   private:
    friend class unordered_map;

    void advance_to_valid() {
      while (bucket_ < map_->bucket_count() && map_->buckets_[bucket_] == nullptr) {
        ++bucket_;
      }
      node_ = (bucket_ < map_->bucket_count()) ? map_->buckets_[bucket_] : nullptr;
    }

    const unordered_map* map_;
    size_type bucket_;
    Node* node_;
  };

  class const_iterator {
   public:
    using value_type = pair<const Key, Value>;
    using reference = const value_type&;
    using pointer = const value_type*;
    using difference_type = ptrdiff_t;

    const_iterator() : map_(nullptr), bucket_(0), node_(nullptr) {}
    const_iterator(const unordered_map* m, size_type b, const Node* n)
        : map_(m), bucket_(b), node_(n) {}
    const_iterator(iterator it) : map_(it.map_), bucket_(it.bucket_), node_(it.node_) {}  // NOLINT

    reference operator*() const { return node_->kv; }
    pointer operator->() const { return &node_->kv; }

    const_iterator& operator++() {
      node_ = node_->next;
      if (node_ == nullptr) {
        ++bucket_;
        advance_to_valid();
      }
      return *this;
    }

    const_iterator operator++(int) {
      const_iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator==(const const_iterator& other) const { return node_ == other.node_; }
    bool operator!=(const const_iterator& other) const { return node_ != other.node_; }

   private:
    friend class unordered_map;

    void advance_to_valid() {
      while (bucket_ < map_->bucket_count() && map_->buckets_[bucket_] == nullptr) {
        ++bucket_;
      }
      node_ = (bucket_ < map_->bucket_count()) ? map_->buckets_[bucket_] : nullptr;
    }

    const unordered_map* map_;
    size_type bucket_;
    const Node* node_;
  };

  // =========================================================================
  // Construction / destruction
  // =========================================================================

  unordered_map() : buckets_(kDefaultBuckets, nullptr), size_(0) {}

  unordered_map(initializer_list<value_type> il) : unordered_map() {
    for (const auto& p : il) {
      insert(p);
    }
  }

  unordered_map(const unordered_map& other) : buckets_(other.buckets_.size(), nullptr), size_(0) {
    for (const auto& p : other) {
      insert(p);
    }
  }

  unordered_map(unordered_map&& other) noexcept
      : buckets_(std::move(other.buckets_)), size_(other.size_) {
    other.buckets_.resize(kDefaultBuckets);
    for (auto& b : other.buckets_) {
      b = nullptr;
    }
    other.size_ = 0;
  }

  ~unordered_map() { clear(); }

  // =========================================================================
  // Assignment
  // =========================================================================

  unordered_map& operator=(const unordered_map& other) {
    if (this != &other) {
      unordered_map tmp(other);
      swap(tmp);
    }
    return *this;
  }

  unordered_map& operator=(unordered_map&& other) noexcept {
    if (this != &other) {
      clear();
      buckets_ = std::move(other.buckets_);
      size_ = other.size_;
      other.buckets_.resize(kDefaultBuckets);
      for (auto& b : other.buckets_) {
        b = nullptr;
      }
      other.size_ = 0;
    }
    return *this;
  }

  // =========================================================================
  // Iterators
  // =========================================================================

  iterator begin() {
    for (size_type i = 0; i < buckets_.size(); ++i) {
      if (buckets_[i] != nullptr) {
        return iterator(this, i, buckets_[i]);
      }
    }
    return end();
  }

  const_iterator begin() const {
    for (size_type i = 0; i < buckets_.size(); ++i) {
      if (buckets_[i] != nullptr) {
        return const_iterator(this, i, buckets_[i]);
      }
    }
    return end();
  }

  iterator end() { return iterator(this, buckets_.size(), nullptr); }
  const_iterator end() const { return const_iterator(this, buckets_.size(), nullptr); }

  // =========================================================================
  // Capacity
  // =========================================================================

  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
  [[nodiscard]] size_type size() const noexcept { return size_; }
  [[nodiscard]] size_type bucket_count() const noexcept { return buckets_.size(); }

  // =========================================================================
  // Lookup
  // =========================================================================

  iterator find(const Key& key) {
    size_type bucket = bucket_for(key);
    for (Node* n = buckets_[bucket]; n != nullptr; n = n->next) {
      if (KeyEqual{}(n->kv.first, key)) {
        return iterator(this, bucket, n);
      }
    }
    return end();
  }

  const_iterator find(const Key& key) const {
    size_type bucket = bucket_for(key);
    for (const Node* n = buckets_[bucket]; n != nullptr; n = n->next) {
      if (KeyEqual{}(n->kv.first, key)) {
        return const_iterator(this, bucket, n);
      }
    }
    return end();
  }

  size_type count(const Key& key) const { return find(key) != end() ? 1 : 0; }

  bool contains(const Key& key) const { return find(key) != end(); }

  Value& operator[](const Key& key) {
    auto it = find(key);
    if (it != end()) {
      return it->second;
    }
    return insert_node(key)->kv.second;
  }

  Value& operator[](Key&& key) {
    auto it = find(key);
    if (it != end()) {
      return it->second;
    }
    return insert_node(std::move(key))->kv.second;
  }

  // =========================================================================
  // Modifiers
  // =========================================================================

  pair<iterator, bool> insert(const value_type& kv) {
    auto it = find(kv.first);
    if (it != end()) {
      return make_pair(it, false);
    }
    maybe_rehash();
    size_type bucket = bucket_for(kv.first);
    Node* node = new Node(kv);
    node->next = buckets_[bucket];
    buckets_[bucket] = node;
    ++size_;
    return make_pair(iterator(this, bucket, node), true);
  }

  template <typename... Args>
  pair<iterator, bool> emplace(Args&&... args) {
    // Construct a temporary node to extract the key.
    Node* node = new Node(std::forward<Args>(args)...);
    auto it = find(node->kv.first);
    if (it != end()) {
      delete node;
      return make_pair(it, false);
    }
    maybe_rehash();
    size_type bucket = bucket_for(node->kv.first);
    node->next = buckets_[bucket];
    buckets_[bucket] = node;
    ++size_;
    return make_pair(iterator(this, bucket, node), true);
  }

  iterator erase(const_iterator pos) {
    size_type bucket = pos.bucket_;
    const Node* target = pos.node_;
    // Advance pos to find the next element before unlinking.
    const_iterator next_it = pos;
    ++next_it;

    Node** prev = &buckets_[bucket];
    while (*prev != nullptr) {
      if (*prev == target) {
        *prev = (*prev)->next;
        delete const_cast<Node*>(target);
        --size_;
        return iterator(this, next_it.bucket_, const_cast<Node*>(next_it.node_));
      }
      prev = &(*prev)->next;
    }
    return end();
  }

  size_type erase(const Key& key) {
    auto it = find(key);
    if (it == end()) {
      return 0;
    }
    erase(const_iterator(it));
    return 1;
  }

  void clear() {
    for (size_type i = 0; i < buckets_.size(); ++i) {
      Node* n = buckets_[i];
      while (n != nullptr) {
        Node* next = n->next;
        delete n;
        n = next;
      }
      buckets_[i] = nullptr;
    }
    size_ = 0;
  }

  void swap(unordered_map& other) noexcept {
    buckets_.swap(other.buckets_);
    std::swap(size_, other.size_);
  }

 private:
  static constexpr size_type kDefaultBuckets = 16;
  static constexpr size_type kMaxLoadFactorNum = 3;  // Load factor threshold = 3/4.
  static constexpr size_type kMaxLoadFactorDen = 4;

  vector<Node*> buckets_;
  size_type size_;

  size_type bucket_for(const Key& key) const { return Hash{}(key) % buckets_.size(); }

  void maybe_rehash() {
    if (size_ * kMaxLoadFactorDen < buckets_.size() * kMaxLoadFactorNum) {
      return;
    }
    size_type new_count = buckets_.size() * 2;
    vector<Node*> new_buckets(new_count, nullptr);
    for (size_type i = 0; i < buckets_.size(); ++i) {
      Node* n = buckets_[i];
      while (n != nullptr) {
        Node* next = n->next;
        size_type b = Hash{}(n->kv.first) % new_count;
        n->next = new_buckets[b];
        new_buckets[b] = n;
        n = next;
      }
    }
    buckets_ = std::move(new_buckets);
  }

  template <typename K>
  Node* insert_node(K&& key) {
    maybe_rehash();
    size_type bucket = Hash{}(key) % buckets_.size();
    Node* node = new Node(std::forward<K>(key));
    node->next = buckets_[bucket];
    buckets_[bucket] = node;
    ++size_;
    return node;
  }
};

}  // namespace std

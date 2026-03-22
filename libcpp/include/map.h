#pragma once

#include <cstddef.h>
#include <initializer_list.h>
#include <iterator.h>
#include <new.h>
#include <utility.h>

namespace std {

// Less-than comparator.
template <typename T>
struct less {
  constexpr bool operator()(const T& a, const T& b) const { return a < b; }
};

// Red-black tree backed ordered map.
template <typename Key, typename Value, typename Compare = less<Key>>
class map {
 public:
  using key_type = Key;
  using mapped_type = Value;
  using value_type = pair<const Key, Value>;
  using size_type = size_t;
  using key_compare = Compare;

 private:
  enum class Color : bool { kRed = false, kBlack = true };

  struct Node {
    value_type kv;
    Node* parent;
    Node* left;
    Node* right;
    Color color;

    template <typename K, typename V>
    Node(K&& k, V&& v)
        : kv(std::forward<K>(k), std::forward<V>(v)),
          parent(nullptr),
          left(nullptr),
          right(nullptr),
          color(Color::kRed) {}

    explicit Node(const value_type& p)
        : kv(p), parent(nullptr), left(nullptr), right(nullptr), color(Color::kRed) {}
  };

 public:
  // Bidirectional in-order iterator.
  class iterator {
   public:
    using iterator_category = bidirectional_iterator_tag;
    using value_type = pair<const Key, Value>;
    using difference_type = ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    iterator() : node_(nullptr), map_(nullptr) {}
    iterator(Node* n, const map* m) : node_(n), map_(m) {}

    reference operator*() const { return node_->kv; }
    pointer operator->() const { return &node_->kv; }

    iterator& operator++() {
      node_ = map_->successor(node_);
      return *this;
    }

    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    iterator& operator--() {
      if (node_ == nullptr) {
        node_ = map_->max_node(map_->root_);
      } else {
        node_ = map_->predecessor(node_);
      }
      return *this;
    }

    iterator operator--(int) {
      iterator tmp = *this;
      --(*this);
      return tmp;
    }

    bool operator==(const iterator& other) const { return node_ == other.node_; }
    bool operator!=(const iterator& other) const { return node_ != other.node_; }

   private:
    friend class map;
    Node* node_;
    const map* map_;
  };

  class const_iterator {
   public:
    using iterator_category = bidirectional_iterator_tag;
    using value_type = pair<const Key, Value>;
    using difference_type = ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;

    const_iterator() : node_(nullptr), map_(nullptr) {}
    const_iterator(const Node* n, const map* m) : node_(n), map_(m) {}
    const_iterator(iterator it) : node_(it.node_), map_(it.map_) {}  // NOLINT

    reference operator*() const { return node_->kv; }
    pointer operator->() const { return &node_->kv; }

    const_iterator& operator++() {
      node_ = map_->successor(const_cast<Node*>(node_));
      return *this;
    }

    const_iterator operator++(int) {
      const_iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    const_iterator& operator--() {
      if (node_ == nullptr) {
        node_ = map_->max_node(map_->root_);
      } else {
        node_ = map_->predecessor(const_cast<Node*>(node_));
      }
      return *this;
    }

    const_iterator operator--(int) {
      const_iterator tmp = *this;
      --(*this);
      return tmp;
    }

    bool operator==(const const_iterator& other) const { return node_ == other.node_; }
    bool operator!=(const const_iterator& other) const { return node_ != other.node_; }

   private:
    const Node* node_;
    const map* map_;
  };

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // =========================================================================
  // Construction / destruction
  // =========================================================================

  map() : root_(nullptr), size_(0) {}

  map(initializer_list<value_type> il) : map() {
    for (const auto& p : il) {
      insert(p);
    }
  }

  map(const map& other) : map() {
    for (const auto& p : other) {
      insert(p);
    }
  }

  map(map&& other) noexcept : root_(other.root_), size_(other.size_) {
    other.root_ = nullptr;
    other.size_ = 0;
  }

  ~map() { destroy_tree(root_); }

  // =========================================================================
  // Assignment
  // =========================================================================

  map& operator=(const map& other) {
    if (this != &other) {
      map tmp(other);
      swap(tmp);
    }
    return *this;
  }

  map& operator=(map&& other) noexcept {
    if (this != &other) {
      destroy_tree(root_);
      root_ = other.root_;
      size_ = other.size_;
      other.root_ = nullptr;
      other.size_ = 0;
    }
    return *this;
  }

  // =========================================================================
  // Iterators
  // =========================================================================

  iterator begin() { return iterator(min_node(root_), this); }
  const_iterator begin() const { return const_iterator(min_node(root_), this); }
  const_iterator cbegin() const { return begin(); }
  iterator end() { return iterator(nullptr, this); }
  const_iterator end() const { return const_iterator(nullptr, this); }
  const_iterator cend() const { return end(); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

  // =========================================================================
  // Capacity
  // =========================================================================

  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
  [[nodiscard]] size_type size() const noexcept { return size_; }

  // =========================================================================
  // Lookup
  // =========================================================================

  iterator find(const Key& key) {
    Node* n = root_;
    while (n != nullptr) {
      if (Compare{}(key, n->kv.first)) {
        n = n->left;
      } else if (Compare{}(n->kv.first, key)) {
        n = n->right;
      } else {
        return iterator(n, this);
      }
    }
    return end();
  }

  const_iterator find(const Key& key) const {
    Node* n = root_;
    while (n != nullptr) {
      if (Compare{}(key, n->kv.first)) {
        n = n->left;
      } else if (Compare{}(n->kv.first, key)) {
        n = n->right;
      } else {
        return const_iterator(n, this);
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
    auto result = insert(value_type(key, Value()));
    return result.first->second;
  }

  Value& operator[](Key&& key) {
    auto it = find(key);
    if (it != end()) {
      return it->second;
    }
    auto result = insert(value_type(std::move(key), Value()));
    return result.first->second;
  }

  // =========================================================================
  // Modifiers
  // =========================================================================

  pair<iterator, bool> insert(const value_type& kv) {
    Node* parent = nullptr;
    Node* cur = root_;
    while (cur != nullptr) {
      parent = cur;
      if (Compare{}(kv.first, cur->kv.first)) {
        cur = cur->left;
      } else if (Compare{}(cur->kv.first, kv.first)) {
        cur = cur->right;
      } else {
        return make_pair(iterator(cur, this), false);
      }
    }
    Node* node = new Node(kv);
    node->parent = parent;
    if (parent == nullptr) {
      root_ = node;
    } else if (Compare{}(kv.first, parent->kv.first)) {
      parent->left = node;
    } else {
      parent->right = node;
    }
    ++size_;
    insert_fixup(node);
    return make_pair(iterator(node, this), true);
  }

  iterator erase(iterator pos) {
    if (pos == end()) {
      return end();
    }
    iterator next = pos;
    ++next;
    delete_node(pos.node_);
    --size_;
    return next;
  }

  size_type erase(const Key& key) {
    auto it = find(key);
    if (it == end()) {
      return 0;
    }
    erase(it);
    return 1;
  }

  void clear() {
    destroy_tree(root_);
    root_ = nullptr;
    size_ = 0;
  }

  void swap(map& other) noexcept {
    std::swap(root_, other.root_);
    std::swap(size_, other.size_);
  }

 private:
  Node* root_;
  size_type size_;

  // =========================================================================
  // Tree navigation
  // =========================================================================

  Node* min_node(Node* n) const {
    if (n == nullptr) {
      return nullptr;
    }
    while (n->left != nullptr) {
      n = n->left;
    }
    return n;
  }

  Node* max_node(Node* n) const {
    if (n == nullptr) {
      return nullptr;
    }
    while (n->right != nullptr) {
      n = n->right;
    }
    return n;
  }

  Node* successor(Node* n) const {
    if (n->right != nullptr) {
      return min_node(n->right);
    }
    Node* p = n->parent;
    while (p != nullptr && n == p->right) {
      n = p;
      p = p->parent;
    }
    return p;
  }

  Node* predecessor(Node* n) const {
    if (n->left != nullptr) {
      return max_node(n->left);
    }
    Node* p = n->parent;
    while (p != nullptr && n == p->left) {
      n = p;
      p = p->parent;
    }
    return p;
  }

  // =========================================================================
  // Red-black tree rotations and fixup
  // =========================================================================

  void rotate_left(Node* x) {
    Node* y = x->right;
    x->right = y->left;
    if (y->left != nullptr) {
      y->left->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == nullptr) {
      root_ = y;
    } else if (x == x->parent->left) {
      x->parent->left = y;
    } else {
      x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
  }

  void rotate_right(Node* x) {
    Node* y = x->left;
    x->left = y->right;
    if (y->right != nullptr) {
      y->right->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == nullptr) {
      root_ = y;
    } else if (x == x->parent->right) {
      x->parent->right = y;
    } else {
      x->parent->left = y;
    }
    y->right = x;
    x->parent = y;
  }

  static Color color_of(Node* n) { return n == nullptr ? Color::kBlack : n->color; }

  void insert_fixup(Node* z) {
    while (z != root_ && color_of(z->parent) == Color::kRed) {
      if (z->parent == z->parent->parent->left) {
        Node* uncle = z->parent->parent->right;
        if (color_of(uncle) == Color::kRed) {
          z->parent->color = Color::kBlack;
          uncle->color = Color::kBlack;
          z->parent->parent->color = Color::kRed;
          z = z->parent->parent;
        } else {
          if (z == z->parent->right) {
            z = z->parent;
            rotate_left(z);
          }
          z->parent->color = Color::kBlack;
          z->parent->parent->color = Color::kRed;
          rotate_right(z->parent->parent);
        }
      } else {
        Node* uncle = z->parent->parent->left;
        if (color_of(uncle) == Color::kRed) {
          z->parent->color = Color::kBlack;
          uncle->color = Color::kBlack;
          z->parent->parent->color = Color::kRed;
          z = z->parent->parent;
        } else {
          if (z == z->parent->left) {
            z = z->parent;
            rotate_right(z);
          }
          z->parent->color = Color::kBlack;
          z->parent->parent->color = Color::kRed;
          rotate_left(z->parent->parent);
        }
      }
    }
    root_->color = Color::kBlack;
  }

  // Transplant subtree rooted at v into u's position.
  void transplant(Node* u, Node* v) {
    if (u->parent == nullptr) {
      root_ = v;
    } else if (u == u->parent->left) {
      u->parent->left = v;
    } else {
      u->parent->right = v;
    }
    if (v != nullptr) {
      v->parent = u->parent;
    }
  }

  void delete_node(Node* z) {
    Node* y = z;
    Color y_original_color = y->color;
    Node* x = nullptr;
    Node* x_parent = nullptr;

    if (z->left == nullptr) {
      x = z->right;
      x_parent = z->parent;
      transplant(z, z->right);
    } else if (z->right == nullptr) {
      x = z->left;
      x_parent = z->parent;
      transplant(z, z->left);
    } else {
      y = min_node(z->right);
      y_original_color = y->color;
      x = y->right;
      if (y->parent == z) {
        x_parent = y;
      } else {
        x_parent = y->parent;
        transplant(y, y->right);
        y->right = z->right;
        y->right->parent = y;
      }
      transplant(z, y);
      y->left = z->left;
      y->left->parent = y;
      y->color = z->color;
    }
    delete z;
    if (y_original_color == Color::kBlack) {
      delete_fixup(x, x_parent);
    }
  }

  void delete_fixup(Node* x, Node* x_parent) {
    while (x != root_ && color_of(x) == Color::kBlack) {
      if (x == x_parent->left) {
        Node* w = x_parent->right;
        if (color_of(w) == Color::kRed) {
          w->color = Color::kBlack;
          x_parent->color = Color::kRed;
          rotate_left(x_parent);
          w = x_parent->right;
        }
        if (color_of(w->left) == Color::kBlack && color_of(w->right) == Color::kBlack) {
          w->color = Color::kRed;
          x = x_parent;
          x_parent = x->parent;
        } else {
          if (color_of(w->right) == Color::kBlack) {
            if (w->left != nullptr) {
              w->left->color = Color::kBlack;
            }
            w->color = Color::kRed;
            rotate_right(w);
            w = x_parent->right;
          }
          w->color = x_parent->color;
          x_parent->color = Color::kBlack;
          if (w->right != nullptr) {
            w->right->color = Color::kBlack;
          }
          rotate_left(x_parent);
          x = root_;
        }
      } else {
        Node* w = x_parent->left;
        if (color_of(w) == Color::kRed) {
          w->color = Color::kBlack;
          x_parent->color = Color::kRed;
          rotate_right(x_parent);
          w = x_parent->left;
        }
        if (color_of(w->right) == Color::kBlack && color_of(w->left) == Color::kBlack) {
          w->color = Color::kRed;
          x = x_parent;
          x_parent = x->parent;
        } else {
          if (color_of(w->left) == Color::kBlack) {
            if (w->right != nullptr) {
              w->right->color = Color::kBlack;
            }
            w->color = Color::kRed;
            rotate_left(w);
            w = x_parent->left;
          }
          w->color = x_parent->color;
          x_parent->color = Color::kBlack;
          if (w->left != nullptr) {
            w->left->color = Color::kBlack;
          }
          rotate_right(x_parent);
          x = root_;
        }
      }
    }
    if (x != nullptr) {
      x->color = Color::kBlack;
    }
  }

  void destroy_tree(Node* n) {
    if (n == nullptr) {
      return;
    }
    destroy_tree(n->left);
    destroy_tree(n->right);
    delete n;
  }
};

}  // namespace std

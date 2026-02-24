#include <stdint.h>
#include <string.h>

#include "heap.h"
#include "ktest.h"

// ---------------------------------------------------------------------------
// kmalloc
// ---------------------------------------------------------------------------

TEST(heap, alloc_returns_nonnull) {
  void* p = kmalloc(1);
  ASSERT_NOT_NULL(p);
  kfree(p);
}

TEST(heap, alloc_is_16byte_aligned) {
  const size_t sizes[] = {1, 7, 16, 17, 64, 100, 256};
  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    void* p = kmalloc(sizes[i]);
    ASSERT_NOT_NULL(p);
    ASSERT((reinterpret_cast<uintptr_t>(p) & 15u) == 0u);
    kfree(p);
  }
}

TEST(heap, alloc_writable) {
  uint8_t* p = static_cast<uint8_t*>(kmalloc(64));
  ASSERT_NOT_NULL(p);
  for (int i = 0; i < 64; ++i) p[i] = static_cast<uint8_t>(i);
  for (int i = 0; i < 64; ++i) ASSERT_EQ(p[i], static_cast<uint8_t>(i));
  kfree(p);
}

TEST(heap, alloc_multiple_disjoint) {
  char* a = static_cast<char*>(kmalloc(64));
  char* b = static_cast<char*>(kmalloc(64));
  ASSERT_NOT_NULL(a);
  ASSERT_NOT_NULL(b);
  ASSERT(b >= a + 64 || a >= b + 64);
  kfree(a);
  kfree(b);
}

TEST(heap, alloc_zero_returns_null) {
  void* p = kmalloc(0);
  ASSERT_NULL(p);
}

TEST(heap, alloc_in_grown_region_is_aligned) {
  // Trigger growth, then check alignment of an allocation in the new region.
  static constexpr size_t BIG = 128u * 1024u;
  void* a = kmalloc(BIG);
  ASSERT_NOT_NULL(a);
  void* b = kmalloc(32);
  ASSERT_NOT_NULL(b);
  ASSERT((reinterpret_cast<uintptr_t>(b) & 15u) == 0u);
  kfree(a);
  kfree(b);
}

TEST(heap, many_small_allocs) {
  static constexpr size_t N = 256;
  void* ptrs[N];
  for (size_t i = 0; i < N; ++i) {
    ptrs[i] = kmalloc(16);
    ASSERT_NOT_NULL(ptrs[i]);
  }
  for (size_t i = 0; i < N; ++i) kfree(ptrs[i]);
}

TEST(heap, oom_returns_null) {
  void* p = kmalloc(Heap::kMaxSize + 1);
  ASSERT_NULL(p);
}

// ---------------------------------------------------------------------------
// kcalloc
// ---------------------------------------------------------------------------

TEST(heap, calloc_zeroed) {
  uint8_t* p = static_cast<uint8_t*>(kcalloc(8, 16));
  ASSERT_NOT_NULL(p);
  for (int i = 0; i < 128; ++i) ASSERT_EQ(p[i], static_cast<uint8_t>(0));
  kfree(p);
}

TEST(heap, calloc_overflow_safe) {
  void* p = kcalloc(static_cast<size_t>(-1), 2);
  ASSERT_NULL(p);
}

TEST(heap, calloc_zero_count_returns_null) { ASSERT_NULL(kcalloc(0, 16)); }

TEST(heap, calloc_zero_size_returns_null) { ASSERT_NULL(kcalloc(8, 0)); }

// ---------------------------------------------------------------------------
// kfree
// ---------------------------------------------------------------------------

TEST(heap, free_null_safe) { kfree(nullptr); }

TEST(heap, coalesce_forward) {
  void* a = kmalloc(64);
  void* b = kmalloc(64);
  ASSERT_NOT_NULL(a);
  ASSERT_NOT_NULL(b);
  // Free b first, then a -- forward coalesce merges a into b's space.
  kfree(b);
  kfree(a);
  void* c = kmalloc(128);
  ASSERT_NOT_NULL(c);
  kfree(c);
}

TEST(heap, coalesce_backward) {
  void* a = kmalloc(64);
  void* b = kmalloc(64);
  ASSERT_NOT_NULL(a);
  ASSERT_NOT_NULL(b);
  // Free a first, then b - backward coalesce merges b into a's block.
  kfree(a);
  kfree(b);
  void* c = kmalloc(128);
  ASSERT_NOT_NULL(c);
  kfree(c);
}

TEST(heap, grow_on_demand) {
  // Initial heap is 64 KiB; allocate more than that to trigger growth.
  static constexpr size_t BIG = 128u * 1024u;  // 128 KiB
  void* p = kmalloc(BIG);
  ASSERT_NOT_NULL(p);
  memset(p, 0xAB, BIG);
  auto* bytes = static_cast<uint8_t*>(p);
  ASSERT_EQ(bytes[0], static_cast<uint8_t>(0xAB));
  ASSERT_EQ(bytes[BIG - 1], static_cast<uint8_t>(0xAB));
  kfree(p);
}

TEST(heap, grow_coalesce_adjacent) {
  // Allocate two 1 MiB blocks (each triggers growth), free both,
  // then allocate a 2 MiB block - requires coalescing across blocks.
  static constexpr size_t MB = 1u * 1024u * 1024u;
  void* a = kmalloc(MB);
  void* b = kmalloc(MB);
  ASSERT_NOT_NULL(a);
  ASSERT_NOT_NULL(b);
  kfree(a);
  kfree(b);
  void* c = kmalloc(MB * 2);
  ASSERT_NOT_NULL(c);
  kfree(c);
}

TEST(heap, fragmentation_cycle_allows_large_alloc) {
  // Alternate alloc/free with varying sizes, then verify a large allocation
  // still succeeds after all blocks have been freed and coalesced.
  static constexpr int N = 64;
  void* ptrs[N];
  for (int i = 0; i < N; ++i) {
    ptrs[i] = kmalloc(static_cast<size_t>(16 + (i % 8) * 16));
    ASSERT_NOT_NULL(ptrs[i]);
  }
  for (int i = 0; i < N; i += 2) {
    kfree(ptrs[i]);
  }
  for (int i = 1; i < N; i += 2) {
    kfree(ptrs[i]);
  }
  void* big = kmalloc(4096);
  ASSERT_NOT_NULL(big);
  kfree(big);
}

// ---------------------------------------------------------------------------
// krealloc
// ---------------------------------------------------------------------------

TEST(heap, realloc_null_is_malloc) {
  void* p = krealloc(nullptr, 64);
  ASSERT_NOT_NULL(p);
  kfree(p);
}

TEST(heap, realloc_zero_frees) {
  void* p = kmalloc(64);
  ASSERT_NOT_NULL(p);
  void* q = krealloc(p, 0);
  ASSERT_NULL(q);
}

TEST(heap, realloc_grow_preserves_data) {
  char* p = static_cast<char*>(kmalloc(16));
  ASSERT_NOT_NULL(p);
  memcpy(p, "Hello, kernel!", 15);
  char* q = static_cast<char*>(krealloc(p, 128));
  ASSERT_NOT_NULL(q);
  ASSERT_EQ(memcmp(q, "Hello, kernel!", 15), 0);
  kfree(q);
}

TEST(heap, realloc_shrink_preserves_data) {
  char* p = static_cast<char*>(kmalloc(128));
  ASSERT_NOT_NULL(p);
  for (int i = 0; i < 16; ++i) {
    p[i] = static_cast<char>('A' + i);
  }
  char* q = static_cast<char*>(krealloc(p, 16));
  ASSERT_NOT_NULL(q);
  for (int i = 0; i < 16; ++i) {
    ASSERT_EQ(q[i], static_cast<char>('A' + i));
  }
  kfree(q);
}

TEST(heap, realloc_fit_returns_same_ptr) {
  void* p = kmalloc(128);
  ASSERT_NOT_NULL(p);
  void* q = krealloc(p, 64);
  ASSERT_EQ(p, q);
  kfree(q);
}

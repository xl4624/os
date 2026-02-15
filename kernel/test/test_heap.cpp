#include <stdint.h>
#include <string.h>

#include "heap.hpp"
#include "ktest.hpp"

TEST(heap, alloc_returns_nonnull) {
    void *p = kmalloc(1);
    ASSERT_NOT_NULL(p);
    kfree(p);
}

TEST(heap, alloc_is_16byte_aligned) {
    // Check several allocation sizes all produce 16-byte-aligned pointers.
    const size_t sizes[] = {1, 7, 16, 17, 64, 100, 256};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        void *p = kmalloc(sizes[i]);
        ASSERT_NOT_NULL(p);
        ASSERT((reinterpret_cast<uintptr_t>(p) & 15u) == 0u);
        kfree(p);
    }
}

TEST(heap, alloc_writable) {
    uint8_t *p = static_cast<uint8_t *>(kmalloc(64));
    ASSERT_NOT_NULL(p);
    for (int i = 0; i < 64; ++i)
        p[i] = static_cast<uint8_t>(i);
    for (int i = 0; i < 64; ++i)
        ASSERT_EQ(p[i], static_cast<uint8_t>(i));
    kfree(p);
}

TEST(heap, alloc_multiple_disjoint) {
    char *a = static_cast<char *>(kmalloc(64));
    char *b = static_cast<char *>(kmalloc(64));
    ASSERT_NOT_NULL(a);
    ASSERT_NOT_NULL(b);
    ASSERT(b >= a + 64 || a >= b + 64);
    kfree(a);
    kfree(b);
}

TEST(heap, calloc_zeroed) {
    uint8_t *p = static_cast<uint8_t *>(kcalloc(8, 16));  // 128 bytes
    ASSERT_NOT_NULL(p);
    for (int i = 0; i < 128; ++i)
        ASSERT_EQ(p[i], static_cast<uint8_t>(0));
    kfree(p);
}

TEST(heap, calloc_overflow_safe) {
    void *p = kcalloc(static_cast<size_t>(-1), 2);
    ASSERT_NULL(p);
}

TEST(heap, free_null_safe) {
    kfree(nullptr);  // must not panic
}

TEST(heap, coalesce_adjacent) {
    static constexpr size_t HALF = 1u * 1024u * 1024u;
    void *a = kmalloc(HALF);
    void *b = kmalloc(HALF);
    ASSERT_NOT_NULL(a);
    ASSERT_NOT_NULL(b);
    kfree(a);
    kfree(b);
    // Backward-coalesce: b is absorbed into a's (free) block.
    // Now there should be a single ≥2 MiB free region.
    void *c = kmalloc(HALF * 2);
    ASSERT_NOT_NULL(c);
    kfree(c);
}

TEST(heap, realloc_null_is_malloc) {
    void *p = krealloc(nullptr, 64);
    ASSERT_NOT_NULL(p);
    kfree(p);
}

TEST(heap, realloc_zero_frees) {
    void *p = kmalloc(64);
    ASSERT_NOT_NULL(p);
    void *q = krealloc(p, 0);  // equivalent to free(p)
    ASSERT_NULL(q);
}

TEST(heap, realloc_grow_preserves_data) {
    char *p = static_cast<char *>(kmalloc(16));
    ASSERT_NOT_NULL(p);
    memcpy(p, "Hello, kernel!", 15);
    char *q = static_cast<char *>(krealloc(p, 128));
    ASSERT_NOT_NULL(q);
    ASSERT_EQ(memcmp(q, "Hello, kernel!", 15), 0);
    kfree(q);
}

TEST(heap, realloc_fit_returns_same_ptr) {
    void *p = kmalloc(128);
    ASSERT_NOT_NULL(p);
    void *q = krealloc(p, 64);  // shrink: already fits
    ASSERT_EQ(p, q);
    kfree(q);
}

TEST(heap, oom_returns_null) {
    // Request more bytes than the entire heap; must return nullptr, not panic.
    void *p = kmalloc(HEAP_SIZE + 1);
    ASSERT_NULL(p);
}

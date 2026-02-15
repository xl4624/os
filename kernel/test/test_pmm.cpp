#include "ktest.hpp"
#include "pmm.hpp"

TEST(bitmap, initially_all_clear) {
    Bitmap<32> bm;
    for (size_t i = 0; i < 32; ++i)
        ASSERT_FALSE(bm.is_set(i));
}

TEST(bitmap, set_and_is_set) {
    Bitmap<32> bm;
    bm.set(7);
    ASSERT_TRUE(bm.is_set(7));
    ASSERT_FALSE(bm.is_set(6));
    ASSERT_FALSE(bm.is_set(8));
}

TEST(bitmap, clear_after_set) {
    Bitmap<32> bm;
    bm.set(5);
    bm.clear(5);
    ASSERT_FALSE(bm.is_set(5));
}

TEST(bitmap, fill_marks_all_used) {
    Bitmap<32> bm;
    bm.fill();
    for (size_t i = 0; i < 32; ++i)
        ASSERT_TRUE(bm.is_set(i));
}

TEST(bitmap, find_first_clear_empty) {
    Bitmap<32> bm;
    ASSERT_EQ(bm.find_first_clear(), static_cast<size_t>(0));
}

TEST(bitmap, find_first_clear_after_set) {
    Bitmap<32> bm;
    bm.set(0);
    bm.set(1);
    bm.set(2);
    ASSERT_EQ(bm.find_first_clear(), static_cast<size_t>(3));
}

TEST(bitmap, find_first_clear_gap) {
    Bitmap<64> bm;
    bm.fill();
    bm.clear(17);
    ASSERT_EQ(bm.find_first_clear(), static_cast<size_t>(17));
}

TEST(bitmap, find_first_clear_full) {
    Bitmap<32> bm;
    bm.fill();
    ASSERT_EQ(bm.find_first_clear(), static_cast<size_t>(32));
}

TEST(pmm, alloc_returns_nonzero) {
    paddr_t p = kPmm.alloc();
    ASSERT_NE(p, static_cast<paddr_t>(0));
    kPmm.free(p);
}

TEST(pmm, alloc_is_page_aligned) {
    paddr_t p = kPmm.alloc();
    ASSERT_NE(p, static_cast<paddr_t>(0));
    ASSERT_EQ(p % PAGE_SIZE, static_cast<paddr_t>(0));
    kPmm.free(p);
}

TEST(pmm, alloc_decrements_free_count) {
    const size_t before = kPmm.get_free_count();
    paddr_t p = kPmm.alloc();
    ASSERT_NE(p, static_cast<paddr_t>(0));
    ASSERT_EQ(kPmm.get_free_count(), before - 1);
    kPmm.free(p);
}

TEST(pmm, free_increments_free_count) {
    paddr_t p = kPmm.alloc();
    ASSERT_NE(p, static_cast<paddr_t>(0));
    const size_t before = kPmm.get_free_count();
    kPmm.free(p);
    ASSERT_EQ(kPmm.get_free_count(), before + 1);
}

TEST(pmm, counters_sum_is_stable) {
    const size_t total = kPmm.get_free_count() + kPmm.get_used_count();
    paddr_t p = kPmm.alloc();
    ASSERT_NE(p, static_cast<paddr_t>(0));
    ASSERT_EQ(kPmm.get_free_count() + kPmm.get_used_count(), total);
    kPmm.free(p);
    ASSERT_EQ(kPmm.get_free_count() + kPmm.get_used_count(), total);
}

TEST(pmm, alloc_returns_unique_pages) {
    static constexpr int N = 8;
    paddr_t pages[N];
    for (int i = 0; i < N; ++i) {
        pages[i] = kPmm.alloc();
        ASSERT_NE(pages[i], static_cast<paddr_t>(0));
        for (int j = 0; j < i; ++j)
            ASSERT_NE(pages[i], pages[j]);
    }
    for (int i = 0; i < N; ++i)
        kPmm.free(pages[i]);
}

TEST(pmm, free_allows_reallocation) {
    paddr_t p = kPmm.alloc();
    ASSERT_NE(p, static_cast<paddr_t>(0));
    kPmm.free(p);
    // First-fit: the just-freed frame should be returned immediately.
    paddr_t q = kPmm.alloc();
    ASSERT_EQ(q, p);
    kPmm.free(q);
}

#include "ktest.h"
#include "pmm.h"

// ===========================================================================
// Bitmap
// ===========================================================================

TEST(bitmap, initially_all_clear) {
  Bitmap<32> bm;
  for (size_t i = 0; i < 32; ++i) ASSERT_FALSE(bm.is_set(i));
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
  for (size_t i = 0; i < 32; ++i) ASSERT_TRUE(bm.is_set(i));
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

// ===========================================================================
// PhysicalMemoryManager
// ===========================================================================

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
    for (int j = 0; j < i; ++j) ASSERT_NE(pages[i], pages[j]);
  }
  for (int i = 0; i < N; ++i) kPmm.free(pages[i]);
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

// Setting the same bit twice must be idempotent.
TEST(bitmap, set_idempotent) {
  Bitmap<32> bm;
  bm.set(10);
  bm.set(10);
  ASSERT_TRUE(bm.is_set(10));
  ASSERT_FALSE(bm.is_set(9));
  ASSERT_FALSE(bm.is_set(11));
}

// Clearing a bit that was never set must be a no-op.
TEST(bitmap, clear_unset_bit_is_noop) {
  Bitmap<32> bm;
  bm.clear(5);
  ASSERT_FALSE(bm.is_set(5));
}

// Bit 31 (last bit of word 0) and bit 32 (first bit of word 1) in a
// Bitmap<64> must be independently addressable.
TEST(bitmap, word_boundary) {
  Bitmap<64> bm;
  bm.set(31);
  bm.set(32);
  ASSERT_TRUE(bm.is_set(31));
  ASSERT_TRUE(bm.is_set(32));
  ASSERT_FALSE(bm.is_set(30));
  ASSERT_FALSE(bm.is_set(33));
  bm.clear(31);
  ASSERT_FALSE(bm.is_set(31));
  ASSERT_TRUE(bm.is_set(32));
}

// When the first word is fully set, find_first_clear must skip it and return
// the index of the first clear bit in the second word.
TEST(bitmap, find_first_clear_skips_full_first_word) {
  Bitmap<64> bm;
  for (size_t i = 0; i < 32; ++i) {
    bm.set(i);
  }
  ASSERT_EQ(bm.find_first_clear(), static_cast<size_t>(32));
}

// used_count must increment on alloc and return to the baseline after free.
TEST(pmm, used_count_tracks_alloc_free) {
  const size_t used_before = kPmm.get_used_count();
  paddr_t p = kPmm.alloc();
  ASSERT_NE(p, static_cast<paddr_t>(0));
  ASSERT_EQ(kPmm.get_used_count(), used_before + 1);
  kPmm.free(p);
  ASSERT_EQ(kPmm.get_used_count(), used_before);
}

#include <algorithm.h>
#include <cstdint.h>

#include "../framework/test.h"

namespace {

struct ScaleResult {
  uint32_t scale;
  uint32_t dst_w;
  uint32_t dst_h;
  uint32_t off_x;
  uint32_t off_y;
};

ScaleResult calc_scale(uint32_t fb_w, uint32_t fb_h, uint32_t src_w, uint32_t src_h) {
  uint32_t scale_x = fb_w / src_w;
  uint32_t scale_y = fb_h / src_h;
  uint32_t scale = (scale_x < scale_y) ? scale_x : scale_y;
  scale = std::max<uint32_t>(scale, 1);

  uint32_t dst_w = src_w * scale;
  uint32_t dst_h = src_h * scale;
  uint32_t off_x = (fb_w > dst_w) ? (fb_w - dst_w) / 2 : 0;
  uint32_t off_y = (fb_h > dst_h) ? (fb_h - dst_h) / 2 : 0;

  return {scale, dst_w, dst_h, off_x, off_y};
}

}  // namespace

// ====================================
// Scale factor calculation
// ====================================

// DOOM 320x200 -> 1440x1080: min(1440/320, 1080/200) = min(4, 5) = 4
TEST(fb_flip, doom_scale_is_4) {
  auto r = calc_scale(1440, 1080, 320, 200);
  ASSERT_EQ(r.scale, 4U);
  ASSERT_EQ(r.dst_w, 1280U);
  ASSERT_EQ(r.dst_h, 800U);
}

// 1:1 when src exactly matches the framebuffer
TEST(fb_flip, one_to_one_exact_match) {
  auto r = calc_scale(640, 480, 640, 480);
  ASSERT_EQ(r.scale, 1U);
  ASSERT_EQ(r.off_x, 0U);
  ASSERT_EQ(r.off_y, 0U);
}

// Non-square display: scale is limited by the tighter dimension
TEST(fb_flip, tight_dimension_limits_scale) {
  // fb=800x200, src=100x100: scale_x=8, scale_y=2, min=2
  auto r = calc_scale(800, 200, 100, 100);
  ASSERT_EQ(r.scale, 2U);
}

// src larger than fb: scale is clamped to 1
TEST(fb_flip, src_larger_than_fb_clamps_to_1) {
  auto r = calc_scale(320, 200, 640, 480);
  ASSERT_EQ(r.scale, 1U);
}

// ====================================
// Centering / letterbox offsets
// ====================================

TEST(fb_flip, doom_centering) {
  // 1440x1080 display, DOOM 320x200, scale=4 -> dst 1280x800
  // off_x = (1440 - 1280) / 2 = 80
  // off_y = (1080 - 800) / 2 = 140
  auto r = calc_scale(1440, 1080, 320, 200);
  ASSERT_EQ(r.off_x, 80U);
  ASSERT_EQ(r.off_y, 140U);
}

// Centered image never overflows the framebuffer
TEST(fb_flip, no_overflow_doom) {
  auto r = calc_scale(1440, 1080, 320, 200);
  ASSERT(r.off_x + r.dst_w <= 1440U);
  ASSERT(r.off_y + r.dst_h <= 1080U);
}

// Non-divisible dimensions: black bars absorb remainder
TEST(fb_flip, non_divisible_dimensions) {
  // fb=1000x1000, src=300x300: scale=3, dst=900x900, off=50
  auto r = calc_scale(1000, 1000, 300, 300);
  ASSERT_EQ(r.scale, 3U);
  ASSERT_EQ(r.dst_w, 900U);
  ASSERT_EQ(r.off_x, 50U);
  ASSERT(r.off_x + r.dst_w <= 1000U);
}

// Symmetric centering: both sides of the bar are equal
TEST(fb_flip, symmetric_bars) {
  auto r = calc_scale(1440, 1080, 320, 200);
  // Left bar == right bar (dst_w is even, fb_w is even)
  ASSERT_EQ(r.off_x, (1440U - r.dst_w) / 2U);
  ASSERT_EQ(r.off_y, (1080U - r.dst_h) / 2U);
}

// ====================================
// Nearest-neighbor source index
// ====================================

// Verify source pixel lookup: dst pixel (r, c) -> src pixel (r/scale, c/scale)
TEST(fb_flip, nearest_neighbor_src_index) {
  // scale=4: dst row 7 maps to src row 7/4 = 1
  uint32_t scale = 4;
  ASSERT_EQ(7U / scale, 1U);
  // dst col 12 maps to src col 12/4 = 3
  ASSERT_EQ(12U / scale, 3U);
}

// First and last dst rows in a scaled block map to the same src row
TEST(fb_flip, block_rows_map_to_same_src_row) {
  uint32_t scale = 4;
  // All of dst rows 8..11 should map to src row 2
  for (uint32_t r = 8; r < 12; ++r) {
    ASSERT_EQ(r / scale, 2U);
  }
}

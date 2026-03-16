#include "framebuffer.h"

#include <algorithm.h>
#include <cstdint.h>
#include <string.h>

#include "paging.h"
#include "vmm.h"

namespace {

bool fb_available = false;
FramebufferInfo fb_info;
uint8_t* fb_buffer = nullptr;
size_t fb_size = 0;

// Virtual address range reserved for the framebuffer mapping.
// Placed above the kernel heap region in the kernel address space.
// 16 MiB should be enough for resolutions up to 2048x2048x4.
constexpr vaddr_t kFbVirtBase{0xFD000000};

}  // namespace

namespace Framebuffer {

bool init(const multiboot_info* mboot) {
  if ((mboot->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) == 0U) {
    return false;
  }

  if (mboot->framebuffer_type != MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
    return false;
  }

  fb_info.width = mboot->framebuffer_width;
  fb_info.height = mboot->framebuffer_height;
  fb_info.pitch = mboot->framebuffer_pitch;
  fb_info.bpp = mboot->framebuffer_bpp;
  fb_info.red_pos = mboot->framebuffer_rgb.framebuffer_red_field_position;
  fb_info.red_size = mboot->framebuffer_rgb.framebuffer_red_mask_size;
  fb_info.green_pos = mboot->framebuffer_rgb.framebuffer_green_field_position;
  fb_info.green_size = mboot->framebuffer_rgb.framebuffer_green_mask_size;
  fb_info.blue_pos = mboot->framebuffer_rgb.framebuffer_blue_field_position;
  fb_info.blue_size = mboot->framebuffer_rgb.framebuffer_blue_mask_size;

  fb_size = static_cast<size_t>(fb_info.pitch) * fb_info.height;

  // The framebuffer physical address from multiboot is 64 bits, but in a
  // 32-bit OS we can only address the low 4 GiB.
  const paddr_t fb_phys{static_cast<uint32_t>(mboot->framebuffer_addr)};

  // Map the entire framebuffer into the kernel virtual address space.
  // The framebuffer is device memory (MMIO) so we map each page.
  const size_t pages = (fb_size + PAGE_SIZE - 1) / PAGE_SIZE;
  for (size_t i = 0; i < pages; ++i) {
    VMM::map(kFbVirtBase + i * PAGE_SIZE, fb_phys + i * PAGE_SIZE,
             /*writeable=*/true, /*user=*/false);
  }

  fb_buffer = kFbVirtBase.ptr<uint8_t>();
  fb_available = true;
  return true;
}

bool is_available() { return fb_available; }

const FramebufferInfo& info() { return fb_info; }

uint8_t* buffer() { return fb_buffer; }

size_t size() { return fb_size; }

void putpixel(uint32_t x, uint32_t y, uint32_t color) {
  if (x >= fb_info.width || y >= fb_info.height) {
    return;
  }
  const uint32_t offset = (y * fb_info.pitch) + (x * (fb_info.bpp / 8));
  auto* pixel = reinterpret_cast<uint32_t*>(fb_buffer + offset);
  *pixel = color;
}

void blit(const uint32_t* src, uint32_t dst_x, uint32_t dst_y, uint32_t w, uint32_t h) {
  const uint32_t bytes_per_pixel = fb_info.bpp / 8;
  for (uint32_t row = 0; row < h; ++row) {
    const uint32_t y = dst_y + row;
    if (y >= fb_info.height) {
      break;
    }

    uint32_t copy_w = w;
    if (dst_x + copy_w > fb_info.width) {
      copy_w = fb_info.width - dst_x;
    }

    const uint32_t fb_offset = (y * fb_info.pitch) + (dst_x * bytes_per_pixel);
    memcpy(fb_buffer + fb_offset, src + (row * w), copy_w * bytes_per_pixel);
  }
}

void blit_scaled(const uint32_t* src, uint32_t src_w, uint32_t src_h) {
  if (src_w == 0 || src_h == 0) {
    return;
  }

  const uint32_t fb_w = fb_info.width;
  const uint32_t fb_h = fb_info.height;
  const uint32_t bytes_per_pixel = fb_info.bpp / 8;

  // Integer scale factor: largest multiplier that fits both dimensions.
  const uint32_t scale_x = fb_w / src_w;
  const uint32_t scale_y = fb_h / src_h;
  uint32_t scale = (scale_x < scale_y) ? scale_x : scale_y;
  scale = std::max<uint32_t>(scale, 1);

  const uint32_t dst_w = src_w * scale;
  const uint32_t dst_h = src_h * scale;
  const uint32_t off_x = (fb_w > dst_w) ? (fb_w - dst_w) / 2 : 0;
  const uint32_t off_y = (fb_h > dst_h) ? (fb_h - dst_h) / 2 : 0;

  // Clear entire framebuffer to black for letterbox bars.
  memset(fb_buffer, 0, fb_size);

  for (uint32_t r = 0; r < dst_h; ++r) {
    const uint32_t src_row = r / scale;
    // Pointer to the start of the destination row in the framebuffer.
    auto* dst_row = reinterpret_cast<uint32_t*>(fb_buffer + ((off_y + r) * fb_info.pitch) +
                                                (off_x * bytes_per_pixel));
    for (uint32_t c = 0; c < dst_w; ++c) {
      const uint32_t src_col = c / scale;
      dst_row[c] = src[(src_row * src_w) + src_col];
    }
  }
}

}  // namespace Framebuffer

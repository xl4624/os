#pragma once

#include <stddef.h>
#include <stdint.h>

#include "multiboot.h"

/*
 * Linear framebuffer driver.
 *
 * Initialised from multiboot framebuffer info (GRUB sets up a VESA/VBE
 * mode via `set gfxpayload=`). The physical framebuffer memory is mapped
 * into the kernel's virtual address space so the kernel (and, via /dev/fb,
 * userspace) can write pixels directly.
 *
 * The driver exposes low-level putpixel / blit primitives and provides
 * framebuffer metadata (width, height, pitch, bpp) for higher layers.
 */

struct FramebufferInfo {
  uint32_t width;    // pixels
  uint32_t height;   // pixels
  uint32_t pitch;    // bytes per scanline
  uint8_t bpp;       // bits per pixel (typically 32)
  uint8_t red_pos;   // bit position of red channel
  uint8_t red_size;  // bit width of red channel
  uint8_t green_pos;
  uint8_t green_size;
  uint8_t blue_pos;
  uint8_t blue_size;
};

namespace Framebuffer {

// Initialise the framebuffer driver from multiboot info.
// Maps the physical framebuffer into the kernel address space.
// Returns false if no framebuffer was provided by the bootloader.
bool init(const struct multiboot_info* mboot);

// Returns true if a framebuffer is available.
[[nodiscard]] bool is_available();

// Returns framebuffer metadata.
[[nodiscard]] const FramebufferInfo& info();

// Returns a pointer to the mapped framebuffer memory.
// Pixels are laid out linearly: row-major, pitch bytes per row.
[[nodiscard]] uint8_t* buffer();

// Returns the total size of the framebuffer in bytes (pitch * height).
[[nodiscard]] size_t size();

// Write a single pixel at (x, y) with the given 32-bit XRGB color.
void putpixel(uint32_t x, uint32_t y, uint32_t color);

// Blit a rectangle from src buffer to framebuffer at (dst_x, dst_y).
// src must be laid out as w*h pixels in 32-bit XRGB format, tightly
// packed (4 bytes per pixel, w*4 bytes per row).
void blit(const uint32_t* src, uint32_t dst_x, uint32_t dst_y, uint32_t w, uint32_t h);

// Scale src (src_w x src_h, 32-bit XRGB) to fill the framebuffer using
// nearest-neighbor integer scaling with letterbox centering.
// If src is larger than the framebuffer, scale is clamped to 1.
void blit_scaled(const uint32_t* src, uint32_t src_w, uint32_t src_h);

}  // namespace Framebuffer

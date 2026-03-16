#ifndef _SYS_FB_H
#define _SYS_FB_H

#include <stdint.h>

/*
 * Framebuffer info structure returned by reading /dev/fb.
 *
 * Reading /dev/fb returns this struct first (once), then subsequent reads
 * return raw framebuffer pixel data. Writing to /dev/fb writes pixels
 * directly to the framebuffer at the current offset.
 */
struct fb_info {
  uint32_t width;   /* pixels */
  uint32_t height;  /* pixels */
  uint32_t pitch;   /* bytes per scanline */
  uint32_t size;    /* total framebuffer size in bytes */
  uint8_t bpp;      /* bits per pixel */
  uint8_t red_pos;  /* bit position of red channel */
  uint8_t red_size; /* bit width of red channel */
  uint8_t green_pos;
  uint8_t green_size;
  uint8_t blue_pos;
  uint8_t blue_size;
  uint8_t reserved;
};

// Scale src (w*h pixels, 32-bit XRGB) to the hardware framebuffer.
// Returns 0 on success, -1 on error.
int fb_flip(const void* src, unsigned int w, unsigned int h);

#endif

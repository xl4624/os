#include "terminal.h"

#include <string.h>
#include <sys/param.h>

#include "font8x16.h"
#include "framebuffer.h"

static constexpr size_t TAB_WIDTH = 4;

// ANSI-ordered palette as 32-bit XRGB values.
// Indices 0-7 are normal colors, 8-15 are bright variants.
// clang-format off
static constexpr uint32_t kPalette[16] = {
    0x00000000,  //  0 Black
    0x00AA0000,  //  1 Red
    0x0000AA00,  //  2 Green
    0x00AA5500,  //  3 Yellow (Brown)
    0x000000AA,  //  4 Blue
    0x00AA00AA,  //  5 Magenta
    0x0000AAAA,  //  6 Cyan
    0x00AAAAAA,  //  7 White (Light Grey)
    0x00555555,  //  8 Bright Black (Dark Grey)
    0x00FF5555,  //  9 Bright Red
    0x0055FF55,  // 10 Bright Green
    0x00FFFF55,  // 11 Bright Yellow
    0x005555FF,  // 12 Bright Blue
    0x00FF55FF,  // 13 Bright Magenta
    0x0055FFFF,  // 14 Bright Cyan
    0x00FFFFFF,  // 15 Bright White
};
// clang-format on

Terminal kTerminal;

uint32_t Terminal::color_to_rgb(uint8_t color_index) { return kPalette[color_index & 0x0F]; }

bool Terminal::init() {
  if (!Framebuffer::is_available()) {
    return false;
  }

  const auto& fi = Framebuffer::info();
  fb_ = Framebuffer::buffer();
  pitch_ = fi.pitch;
  bpp_ = fi.bpp;

  active_ = true;
  clear();
  return true;
}

void Terminal::draw_glyph(char c, uint8_t color, size_t row, size_t col) {
  if (!active_) {
    return;
  }

  auto ch = static_cast<uint8_t>(c);
  if (ch >= 128) {
    ch = 0;  // replace non-ASCII with blank
  }

  const uint32_t fg = color_to_rgb(color & 0x0F);
  const uint32_t bg = color_to_rgb((color >> 4) & 0x0F);

  auto px = static_cast<uint32_t>(col * kFontWidth);
  auto py = static_cast<uint32_t>(row * kFontHeight);
  const uint32_t bytes_per_pixel = bpp_ / 8;

  const uint8_t* glyph = kFont8x16[ch];

  for (uint32_t gy = 0; gy < kFontHeight; ++gy) {
    const uint8_t row_bits = glyph[gy];
    uint8_t* dest = fb_ + ((py + gy) * pitch_) + (px * bytes_per_pixel);

    for (uint32_t gx = 0; gx < kFontWidth; ++gx) {
      const uint32_t pixel = ((row_bits & (0x80 >> gx)) != 0) ? fg : bg;
      auto* p = reinterpret_cast<uint32_t*>(dest + (gx * bytes_per_pixel));
      *p = pixel;
    }
  }
}

void Terminal::draw_cursor() {
  if (!cursor_visible_) {
    return;
  }
  // Draw an inverted block at the cursor position.
  auto inv_color = static_cast<uint8_t>(((color_ & 0x0F) << 4) | ((color_ >> 4) & 0x0F));
  char ch = cells_[row_][col_].ch;
  if (ch == 0) {
    ch = ' ';
  }
  draw_glyph(ch, inv_color, row_, col_);
}

void Terminal::erase_cursor() {
  const Cell& cell = cells_[row_][col_];
  char ch = cell.ch;
  if (ch == 0) {
    ch = ' ';
  }
  draw_glyph(ch, cell.color, row_, col_);
}

void Terminal::write(const char* data) {
  erase_cursor();
  while (*data != 0) {
    put_char(*data++);
  }
  draw_cursor();
}

void Terminal::write(std::span<const char> data) {
  erase_cursor();
  for (const char c : data) {
    put_char(c);
  }
  draw_cursor();
}

void Terminal::write_char(char c) {
  erase_cursor();
  put_char(c);
  draw_cursor();
}

void Terminal::put_char(char c) {
  if (esc_state_ == EscState::Escape) {
    if (c == '[') {
      esc_state_ = EscState::Csi;
      csi_private_ = false;
      esc_param_count_ = 0;
      esc_params_.fill(0);
    } else {
      esc_state_ = EscState::Normal;
      put_char(c);
    }
    return;
  }

  if (esc_state_ == EscState::Csi) {
    if (c == '?' && esc_param_count_ == 0 && esc_params_[0] == 0) {
      csi_private_ = true;
    } else if (c >= '0' && c <= '9') {
      esc_params_[esc_param_count_] = static_cast<uint16_t>((esc_params_[esc_param_count_] * 10U) +
                                                            static_cast<uint16_t>(c - '0'));
    } else if (c == ';') {
      if (esc_param_count_ < kMaxCsiParams - 1) {
        ++esc_param_count_;
        esc_params_[esc_param_count_] = 0;
      }
    } else {
      dispatch_csi(c);
      esc_state_ = EscState::Normal;
    }
    return;
  }

  // Normal character processing.
  if (c == '\x1b') {
    esc_state_ = EscState::Escape;
    return;
  }

  if (c == '\n') {
    col_ = 0;
    if (++row_ == kRows) {
      scroll();
    }
  } else if (c == '\b') {
    if (col_ > 0) {
      --col_;
    } else if (row_ > 0) {
      --row_;
      size_t tmp = kColumns - 1;
      while (tmp > 0 && cells_[row_][tmp - 1].ch == 0) {
        --tmp;
      }
      col_ = tmp;
    }
    cells_[row_][col_].ch = 0;
    cells_[row_][col_].color = color_;
    draw_glyph(' ', color_, row_, col_);
  } else if (c == '\t') {
    const size_t spaces = TAB_WIDTH - (col_ % TAB_WIDTH);
    for (size_t i = 0; i < spaces; ++i) {
      put_char(' ');
    }
  } else {
    cells_[row_][col_].ch = c;
    cells_[row_][col_].color = color_;
    draw_glyph(c, color_, row_, col_);
    if (++col_ == kColumns) {
      col_ = 0;
      if (++row_ == kRows) {
        scroll();
      }
    }
  }
}

void Terminal::handle_key(KeyEvent event) {
  if (!event.pressed) {
    return;
  }

  if (event.ascii != 0) {
    erase_cursor();
    put_char(event.ascii);
    draw_cursor();
    return;
  }

  erase_cursor();
  switch (event.key.value()) {
    case Key::Up:
      if (row_ > 0) {
        --row_;
      }
      break;
    case Key::Down:
      if (row_ < kRows - 1) {
        ++row_;
      }
      break;
    case Key::Left:
      if (col_ > 0) {
        --col_;
      } else if (row_ > 0) {
        --row_;
        col_ = kColumns - 1;
      }
      break;
    case Key::Right:
      if (col_ < kColumns - 1) {
        ++col_;
      } else if (row_ < kRows - 1) {
        ++row_;
        col_ = 0;
      }
      break;
    default:
      break;
  }
  draw_cursor();
}

void Terminal::scroll() {
  // Shift all rows up by one in the cell buffer.
  memmove(&cells_[0], &cells_[1], sizeof(Cell) * kColumns * (kRows - 1));

  // Clear the bottom row in the cell buffer.
  for (size_t c = 0; c < kColumns; ++c) {
    cells_[kRows - 1][c] = {0, color_};
  }

  if (active_) {
    // Scroll framebuffer pixels: shift rows up by kFontHeight pixels.
    const uint32_t row_bytes = static_cast<uint32_t>(kFontHeight) * pitch_;
    auto total_rows = static_cast<uint32_t>(kRows - 1);
    memmove(fb_, fb_ + row_bytes, total_rows * row_bytes);

    // Clear the bottom row of pixels with background color.
    const uint32_t bg = color_to_rgb((color_ >> 4) & 0x0F);
    auto start_y = static_cast<uint32_t>((kRows - 1) * kFontHeight);
    for (uint32_t y = start_y; y < start_y + kFontHeight; ++y) {
      auto* row_ptr = reinterpret_cast<uint32_t*>(fb_ + (y * pitch_));
      for (uint32_t x = 0; x < kColumns * kFontWidth; ++x) {
        row_ptr[x] = bg;
      }
      // Clear any remaining pixels to the right of the text area.
      const uint32_t fb_width = Framebuffer::info().width;
      for (auto x = static_cast<uint32_t>(kColumns * kFontWidth); x < fb_width; ++x) {
        row_ptr[x] = bg;
      }
    }
  }

  --row_;
}

void Terminal::clear_line(size_t row) {
  for (size_t c = 0; c < kColumns; ++c) {
    cells_[row][c] = {0, color_};
    draw_glyph(' ', color_, row, c);
  }
}

void Terminal::set_color(uint8_t color) { color_ = color; }

void Terminal::clear() {
  color_ = 0x07;  // light grey on black

  // Clear all cells and fill the framebuffer with the background color.
  const uint32_t bg = color_to_rgb((color_ >> 4) & 0x0F);
  for (auto& row : cells_) {
    for (Cell& cell : row) {
      cell = {0, color_};
    }
  }

  // Fill the entire framebuffer with background.
  if (active_) {
    const uint32_t fb_width = Framebuffer::info().width;
    const uint32_t fb_height = Framebuffer::info().height;
    for (uint32_t y = 0; y < fb_height; ++y) {
      auto* row_ptr = reinterpret_cast<uint32_t*>(fb_ + (y * pitch_));
      for (uint32_t x = 0; x < fb_width; ++x) {
        row_ptr[x] = bg;
      }
    }
  }

  row_ = 0;
  col_ = 0;
}

void Terminal::set_position(size_t row, size_t col) {
  erase_cursor();
  if (row >= kRows) {
    row = kRows - 1;
  }
  if (col >= kColumns) {
    col = kColumns - 1;
  }
  row_ = row;
  col_ = col;
  draw_cursor();
}

void Terminal::dispatch_csi(char final_byte) {
  const uint16_t p0 = esc_params_[0];
  const uint16_t p1 = esc_params_[1];
  const size_t n = (p0 == 0) ? 1U : static_cast<size_t>(p0);

  switch (final_byte) {
    case 'H':
    case 'f': {
      auto row = static_cast<size_t>((p0 == 0 ? 1 : p0) - 1);
      auto col = static_cast<size_t>((p1 == 0 ? 1 : p1) - 1);
      if (row >= kRows) {
        row = kRows - 1;
      }
      if (col >= kColumns) {
        col = kColumns - 1;
      }
      row_ = row;
      col_ = col;
      break;
    }
    case 'A':
      row_ -= MIN(n, row_);
      break;
    case 'B':
      row_ = MIN(row_ + n, kRows - 1U);
      break;
    case 'C':
      col_ = MIN(col_ + n, kColumns - 1U);
      break;
    case 'D':
      col_ -= MIN(n, col_);
      break;
    case 'J':
      if (p0 == 2) {
        clear();
      }
      break;
    case 'h':
      if (csi_private_ && p0 == 25) {
        cursor_visible_ = true;
      }
      break;
    case 'l':
      if (csi_private_ && p0 == 25) {
        cursor_visible_ = false;
      }
      break;
    case 'm':
      apply_sgr(static_cast<uint8_t>(esc_param_count_ + 1U));
      break;
    default:
      break;
  }
}

void Terminal::apply_sgr(uint8_t nparams) {
  uint8_t fg = color_ & 0x0FU;
  uint8_t bg = (color_ >> 4) & 0x0FU;

  static constexpr uint8_t kAnsiToVgaFg[8] = {0, 4, 2, 6, 1, 5, 3, 7};
  static constexpr uint8_t kAnsiToVgaBg[8] = {0, 4, 2, 6, 1, 5, 3, 7};

  for (uint8_t i = 0; i < nparams; ++i) {
    const uint16_t code = esc_params_[i];
    if (code == 0) {
      fg = 7;
      bg = 0;
    } else if (code >= 30 && code <= 37) {
      fg = kAnsiToVgaFg[code - 30];
    } else if (code >= 40 && code <= 47) {
      bg = kAnsiToVgaBg[code - 40];
    } else if (code >= 90 && code <= 97) {
      fg = static_cast<uint8_t>(code - 90 + 8);
    } else if (code >= 100 && code <= 107) {
      bg = static_cast<uint8_t>(code - 100 + 8);
    }
  }

  color_ = static_cast<uint8_t>(fg | (bg << 4));
}

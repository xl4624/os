#include "terminal.h"

#include <algorithm.h>
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

  rows_ = std::min(fi.height / kFontHeight, kMaxRows);
  cols_ = std::min(fi.width / kFontWidth, kMaxColumns);

  active_ = true;
  clear();
  return true;
}

void Terminal::draw_glyph(char c, uint8_t color, size_t row, size_t col) {
  if (!active_ || scroll_offset_ > 0) {
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
  char ch = cells_[cursor_.row][cursor_.col].ch;
  if (ch == 0) {
    ch = ' ';
  }
  draw_glyph(ch, inv_color, cursor_.row, cursor_.col);
}

void Terminal::erase_cursor() {
  const Cell& cell = cells_[cursor_.row][cursor_.col];
  char ch = cell.ch;
  if (ch == 0) {
    ch = ' ';
  }
  draw_glyph(ch, cell.color, cursor_.row, cursor_.col);
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

  if (c == '\r') {
    cursor_.col = 0;
  } else if (c == '\n') {
    cursor_.col = 0;
    if (++cursor_.row == rows_) {
      scroll();
    }
  } else if (c == '\b') {
    if (cursor_.col > 0) {
      --cursor_.col;
    } else if (cursor_.row > 0) {
      --cursor_.row;
      size_t tmp = cols_ - 1;
      while (tmp > 0 && cells_[cursor_.row][tmp - 1].ch == 0) {
        --tmp;
      }
      cursor_.col = tmp;
    }
    cells_[cursor_.row][cursor_.col].ch = 0;
    cells_[cursor_.row][cursor_.col].color = color_;
    draw_glyph(' ', color_, cursor_.row, cursor_.col);
  } else if (c == '\t') {
    const size_t spaces = TAB_WIDTH - (cursor_.col % TAB_WIDTH);
    for (size_t i = 0; i < spaces; ++i) {
      put_char(' ');
    }
  } else {
    cells_[cursor_.row][cursor_.col].ch = c;
    cells_[cursor_.row][cursor_.col].color = color_;
    draw_glyph(c, color_, cursor_.row, cursor_.col);
    if (++cursor_.col == cols_) {
      cursor_.col = 0;
      if (++cursor_.row == rows_) {
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
      if (cursor_.row > 0) {
        --cursor_.row;
      }
      break;
    case Key::Down:
      if (cursor_.row < rows_ - 1) {
        ++cursor_.row;
      }
      break;
    case Key::Left:
      if (cursor_.col > 0) {
        --cursor_.col;
      } else if (cursor_.row > 0) {
        --cursor_.row;
        cursor_.col = cols_ - 1;
      }
      break;
    case Key::Right:
      if (cursor_.col < cols_ - 1) {
        ++cursor_.col;
      } else if (cursor_.row < rows_ - 1) {
        ++cursor_.row;
        cursor_.col = 0;
      }
      break;
    default:
      break;
  }
  draw_cursor();
}

void Terminal::scroll() {
  // Save the top row into the scrollback ring buffer before shifting it out.
  const size_t slot = scrollback_head_;
  for (size_t c = 0; c < cols_; ++c) {
    scrollback_[slot][c] = cells_[0][c];
  }
  scrollback_head_ = (scrollback_head_ + 1) % kScrollbackLines;
  if (lines_scrolled_ < kScrollbackLines) {
    ++lines_scrolled_;
  }

  // Shift all rows up by one in the live cell buffer.
  memmove(&cells_[0], &cells_[1], sizeof(Cell) * cols_ * (rows_ - 1));

  // Clear the bottom row in the cell buffer.
  for (size_t c = 0; c < cols_; ++c) {
    cells_[rows_ - 1][c] = {.ch = 0, .color = color_};
  }

  // Only update the framebuffer if the user is viewing the live screen.
  if (active_ && scroll_offset_ == 0) {
    // Scroll framebuffer pixels: shift rows up by kFontHeight pixels.
    const uint32_t row_bytes = static_cast<uint32_t>(kFontHeight) * pitch_;
    auto total_rows = static_cast<uint32_t>(rows_ - 1);
    memmove(fb_, fb_ + row_bytes, total_rows * row_bytes);

    // Clear the bottom row of pixels with background color.
    const uint32_t bg = color_to_rgb((color_ >> 4) & 0x0F);
    auto start_y = static_cast<uint32_t>((rows_ - 1) * kFontHeight);
    for (uint32_t y = start_y; y < start_y + kFontHeight; ++y) {
      auto* row_ptr = reinterpret_cast<uint32_t*>(fb_ + (y * pitch_));
      for (uint32_t x = 0; x < cols_ * kFontWidth; ++x) {
        row_ptr[x] = bg;
      }
      // Clear any remaining pixels to the right of the text area.
      const uint32_t fb_width = Framebuffer::info().width;
      for (auto x = static_cast<uint32_t>(cols_ * kFontWidth); x < fb_width; ++x) {
        row_ptr[x] = bg;
      }
    }
  }

  --cursor_.row;
}

void Terminal::redraw_all() {
  if (!active_) {
    return;
  }
  // Draw rows_ rows onto the framebuffer from history + live buffers.
  // For screen row r (0=top), content is taken from N lines before the live
  // bottom, where N = (rows_ - 1 - r) + scroll_offset_.
  //   N < rows_         => live cells_[rows_ - 1 - N] = cells_[r - scroll_offset_]
  //   N >= rows_        => scrollback, hist_back = N - rows_ entries back from newest
  for (size_t r = 0; r < rows_; ++r) {
    const size_t offset_from_bottom = (rows_ - 1U - r) + scroll_offset_;
    const Cell* src_row = nullptr;
    Cell blank_row[kMaxColumns];
    if (offset_from_bottom < rows_) {
      src_row = cells_[rows_ - 1U - offset_from_bottom];
    } else {
      const size_t hist_back = offset_from_bottom - rows_;  // 0 = most recently scrolled
      if (hist_back < lines_scrolled_) {
        const size_t idx =
            (scrollback_head_ + kScrollbackLines - 1U - hist_back) % kScrollbackLines;
        src_row = scrollback_[idx];
      } else {
        // Beyond available history; draw a blank row.
        for (auto& c : blank_row) {
          c = {.ch = 0, .color = 0x07};
        }
        src_row = blank_row;
      }
    }
    for (size_t c = 0; c < cols_; ++c) {
      const char ch = src_row[c].ch != 0 ? src_row[c].ch : ' ';
      // Temporarily clear scroll_offset_ so draw_glyph actually draws.
      const size_t saved = scroll_offset_;
      scroll_offset_ = 0;
      draw_glyph(ch, src_row[c].color, r, c);
      scroll_offset_ = saved;
    }
  }
}

void Terminal::scroll_view(int delta) {
  if (delta > 0) {
    const auto up = static_cast<size_t>(delta);
    scroll_offset_ += up;
    scroll_offset_ = std::min(scroll_offset_, lines_scrolled_);
  } else if (delta < 0) {
    const auto down = static_cast<size_t>(-delta);
    scroll_offset_ = (down >= scroll_offset_) ? 0U : scroll_offset_ - down;
  }
  redraw_all();
}

void Terminal::clear_line(size_t row) {
  for (size_t c = 0; c < cols_; ++c) {
    cells_[row][c] = {.ch = 0, .color = color_};
    draw_glyph(' ', color_, row, c);
  }
}

void Terminal::set_color(uint8_t color) { color_ = color; }

void Terminal::clear() {
  color_ = 0x07;  // light grey on black
  scroll_offset_ = 0;

  // Clear all cells and fill the framebuffer with the background color.
  const uint32_t bg = color_to_rgb((color_ >> 4) & 0x0F);
  for (auto& row : cells_) {
    for (Cell& cell : row) {
      cell = {.ch = 0, .color = color_};
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

  cursor_.row = 0;
  cursor_.col = 0;
}

void Terminal::set_position(size_t row, size_t col) {
  erase_cursor();
  if (row >= rows_) {
    row = rows_ - 1;
  }
  if (col >= cols_) {
    col = cols_ - 1;
  }
  cursor_.row = row;
  cursor_.col = col;
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
      if (row >= rows_) {
        row = rows_ - 1;
      }
      if (col >= cols_) {
        col = cols_ - 1;
      }
      cursor_.row = row;
      cursor_.col = col;
      break;
    }
    case 'A':
      cursor_.row -= MIN(n, cursor_.row);
      break;
    case 'B':
      cursor_.row = MIN(cursor_.row + n, rows_ - 1U);
      break;
    case 'C':
      cursor_.col = MIN(cursor_.col + n, cols_ - 1U);
      break;
    case 'D':
      cursor_.col -= MIN(n, cursor_.col);
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
    case 'K':
      // Erase from cursor to end of line (only EL 0 / default supported).
      if (p0 == 0) {
        for (size_t c = cursor_.col; c < cols_; ++c) {
          cells_[cursor_.row][c] = {.ch = 0, .color = color_};
          draw_glyph(' ', color_, cursor_.row, c);
        }
      }
      break;
    case 'G': {
      // Cursor horizontal absolute: move to column n (1-based).
      auto new_col = static_cast<size_t>((p0 == 0 ? 1U : static_cast<size_t>(p0)) - 1U);
      if (new_col >= cols_) {
        new_col = cols_ - 1U;
      }
      cursor_.col = new_col;
      break;
    }
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

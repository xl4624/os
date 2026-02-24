#include "tty.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/io.h>
#include <sys/param.h>

static constexpr size_t TAB_WIDTH = 4;

// Maps ANSI color indices 0-7 to VGA color values.
// Bright variants (ANSI 90-97 fg / 100-107 bg) are these values + 8.
static constexpr uint8_t kAnsiToVga[8] = {
    0,  // Black   (30/40)
    4,  // Red     (31/41)
    2,  // Green   (32/42)
    6,  // Yellow  (33/43)
    1,  // Blue    (34/44)
    5,  // Magenta (35/45)
    3,  // Cyan    (36/46)
    7,  // White   (37/47)
};

[[nodiscard]] static constexpr size_t to_index(size_t row, size_t col) {
  return row * VGA::WIDTH + col;
}

// The global VGA text-mode terminal instance.
Terminal kTerminal;

Terminal::Terminal() {
  esc_state_ = EscState::Normal;
  esc_param_count_ = 0;
  for (size_t i = 0; i < kMaxCsiParams; ++i) {
    esc_params_[i] = 0;
  }
  for (size_t y = 0; y < VGA::HEIGHT; ++y) {
    clear_line(y);
  }
  enable_cursor();
}

void Terminal::write(const char* data) {
  while (*data) {
    put_char(*data++);
  }
}

void Terminal::put_char(char c) {
  if (esc_state_ == EscState::Escape) {
    if (c == '[') {
      esc_state_ = EscState::Csi;
      esc_param_count_ = 0;
      for (size_t i = 0; i < kMaxCsiParams; ++i) {
        esc_params_[i] = 0;
      }
    } else {
      // Not a CSI sequence: discard ESC and re-process c normally.
      esc_state_ = EscState::Normal;
      put_char(c);
    }
    return;
  }

  if (esc_state_ == EscState::Csi) {
    if (c >= '0' && c <= '9') {
      esc_params_[esc_param_count_] = static_cast<uint16_t>(esc_params_[esc_param_count_] * 10u +
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

  // EscState::Normal processing.
  if (c == '\x1b') {
    esc_state_ = EscState::Escape;
    return;
  }

  if (c == '\n') {
    col_ = 0;
    if (++row_ == VGA::HEIGHT) {
      scroll();
    }
  } else if (c == '\b') {
    if (col_ > 0) {
      --col_;
    } else if (row_ > 0) {
      --row_;
      size_t tmp = VGA::WIDTH - 1;
      while (tmp > 0 && (buffer_[to_index(row_, tmp - 1)] & 0xFF) == 0) {
        --tmp;
      }
      col_ = tmp;
    }
    put_entry_at(0, color_, row_, col_);
    update_cursor();
  } else if (c == '\t') {
    const size_t spaces = TAB_WIDTH - (col_ % TAB_WIDTH);
    for (size_t i = 0; i < spaces; ++i) {
      put_char(' ');
    }
  } else {
    put_entry_at(c, color_, row_, col_);
    if (++col_ == VGA::WIDTH) {
      col_ = 0;
      if (++row_ == VGA::HEIGHT) {
        scroll();
      }
    }
  }
  update_cursor();
}

void Terminal::handle_key(KeyEvent event) {
  if (!event.pressed) {
    return;
  }

  if (event.ascii) {
    put_char(event.ascii);
    return;
  }

  switch (event.key.value()) {
    case Key::Up:
      if (row_ > 0) {
        --row_;
        update_cursor();
      }
      break;
    case Key::Down:
      if (row_ < VGA::HEIGHT - 1) {
        ++row_;
        update_cursor();
      }
      break;
    case Key::Left:
      if (col_ > 0) {
        --col_;
        update_cursor();
      } else if (row_ > 0) {
        --row_;
        col_ = VGA::WIDTH - 1;
        update_cursor();
      }
      break;
    case Key::Right:
      if (col_ < VGA::WIDTH - 1) {
        ++col_;
        update_cursor();
      } else if (row_ < VGA::HEIGHT - 1) {
        ++row_;
        col_ = 0;
        update_cursor();
      }
      break;
    default:
      break;
  }
}

void Terminal::put_entry_at(char c, uint8_t color, size_t row, size_t col) {
  assert(row < VGA::HEIGHT && "Terminal::put_entry_at(): row out of range");
  assert(col < VGA::WIDTH && "Terminal::put_entry_at(): col out of range");
  buffer_[to_index(row, col)] = VGA::entry(c, color);
}

void Terminal::scroll() {
  assert(row_ == VGA::HEIGHT && "Terminal::scroll(): called when row_ != VGA::HEIGHT");
  for (size_t r = 1; r < VGA::HEIGHT; ++r) {
    for (size_t c = 0; c < VGA::WIDTH; ++c) {
      buffer_[to_index((r - 1), c)] = buffer_[to_index(r, c)];
    }
  }

  // clear bottom line
  clear_line(VGA::HEIGHT - 1);
  --row_;
  update_cursor();
}

void Terminal::clear_line(size_t row) {
  assert(row < VGA::HEIGHT && "Terminal::clear_line(): row out of range");
  for (size_t col = 0; col < VGA::WIDTH; ++col) {
    put_entry_at(0, color_, row, col);
  }
}

void Terminal::set_color(uint8_t color) { color_ = color; }

void Terminal::clear() {
  color_ = VGA::entry_color(VGA::Color::LightGrey, VGA::Color::Black);
  for (size_t y = 0; y < VGA::HEIGHT; ++y) {
    clear_line(y);
  }
  row_ = 0;
  col_ = 0;
  update_cursor();
}

void Terminal::set_position(size_t row, size_t col) {
  if (row >= VGA::HEIGHT) {
    row = VGA::HEIGHT - 1;
  }
  if (col >= VGA::WIDTH) {
    col = VGA::WIDTH - 1;
  }
  row_ = row;
  col_ = col;
  update_cursor();
}

void Terminal::enable_cursor() {
  cursor_enabled_ = true;
  outb(VGA::CRTC_ADDR_REG, VGA::CURSOR_START_REG);
  outb(VGA::CRTC_DATA_REG,
       (inb(VGA::CRTC_DATA_REG) & VGA::CURSOR_START_MASK) | VGA::DEFAULT_CURSOR_START);
  outb(VGA::CRTC_ADDR_REG, VGA::CURSOR_END_REG);
  outb(VGA::CRTC_DATA_REG,
       (inb(VGA::CRTC_DATA_REG) & VGA::CURSOR_END_MASK) | VGA::DEFAULT_CURSOR_END);
}

void Terminal::disable_cursor() {
  cursor_enabled_ = false;
  outb(VGA::CRTC_ADDR_REG, VGA::CURSOR_START_REG);
  outb(VGA::CRTC_DATA_REG, VGA::CURSOR_DISABLE_BIT);
}

void Terminal::update_cursor() {
  if (!cursor_enabled_) {
    return;
  }
  move_cursor(row_, col_);
}

void Terminal::move_cursor(size_t row, size_t col) {
  assert(row < VGA::HEIGHT && "Terminal::move_cursor(): row out of range");
  assert(col < VGA::WIDTH && "Terminal::move_cursor(): col out of range");
  uint16_t pos = static_cast<uint16_t>(row * VGA::WIDTH + col);
  outb(VGA::CRTC_ADDR_REG, VGA::CURSOR_LOC_LOW_REG);
  outb(VGA::CRTC_DATA_REG, static_cast<uint8_t>(pos & 0xFF));
  outb(VGA::CRTC_ADDR_REG, VGA::CURSOR_LOC_HIGH_REG);
  outb(VGA::CRTC_DATA_REG, static_cast<uint8_t>((pos >> 8) & 0xFF));
}

void Terminal::dispatch_csi(char final_byte) {
  uint16_t p0 = esc_params_[0];
  uint16_t p1 = esc_params_[1];
  size_t n = (p0 == 0) ? 1u : static_cast<size_t>(p0);

  switch (final_byte) {
    case 'H':
    case 'f': {
      size_t row = static_cast<size_t>((p0 == 0 ? 1 : p0) - 1);
      size_t col = static_cast<size_t>((p1 == 0 ? 1 : p1) - 1);
      set_position(row, col);
      break;
    }
    case 'A':
      row_ -= MIN(n, row_);
      update_cursor();
      break;
    case 'B':
      row_ = MIN(row_ + n, VGA::HEIGHT - 1u);
      update_cursor();
      break;
    case 'C':
      col_ = MIN(col_ + n, VGA::WIDTH - 1u);
      update_cursor();
      break;
    case 'D':
      col_ -= MIN(n, col_);
      update_cursor();
      break;
    case 'J':
      if (p0 == 2) {
        clear();
      }
      break;
    case 'm':
      apply_sgr(static_cast<uint8_t>(esc_param_count_ + 1u));
      break;
    default:
      break;
  }
}

void Terminal::apply_sgr(uint8_t nparams) {
  uint8_t fg = color_ & 0x0Fu;
  uint8_t bg = (color_ >> 4) & 0x0Fu;

  for (uint8_t i = 0; i < nparams; ++i) {
    uint16_t code = esc_params_[i];
    if (code == 0) {
      fg = static_cast<uint8_t>(VGA::Color::LightGrey);
      bg = static_cast<uint8_t>(VGA::Color::Black);
    } else if (code >= 30 && code <= 37) {
      fg = kAnsiToVga[code - 30];
    } else if (code >= 40 && code <= 47) {
      bg = kAnsiToVga[code - 40];
    } else if (code >= 90 && code <= 97) {
      fg = static_cast<uint8_t>(kAnsiToVga[code - 90] + 8u);
    } else if (code >= 100 && code <= 107) {
      bg = static_cast<uint8_t>(kAnsiToVga[code - 100] + 8u);
    }
  }

  color_ = VGA::entry_color(static_cast<VGA::Color>(fg), static_cast<VGA::Color>(bg));
}

__BEGIN_DECLS

void terminal_write(const char* data) { kTerminal.write(data); }

void terminal_putchar(char c) { kTerminal.put_char(c); }

void terminal_clear(void) { kTerminal.clear(); }

void terminal_set_position(unsigned int row, unsigned int col) { kTerminal.set_position(row, col); }

void terminal_set_color(unsigned char color) { kTerminal.set_color(color); }

__END_DECLS

#pragma once

#include <array.h>
#include <span.h>
#include <stddef.h>
#include <stdint.h>

#include "keyboard.h"

// Framebuffer-based text console.
//
// Renders a dynamically-sized character grid (computed from the framebuffer
// resolution and 8x16 bitmap font) onto the linear framebuffer.
class Terminal {
 public:
  static constexpr size_t kFontWidth = 8;
  static constexpr size_t kFontHeight = 16;
  // Maximum grid dimensions (for static array sizing). Actual usable
  // dimensions are computed from the framebuffer at init() time and
  // accessible via rows() / cols().
  static constexpr size_t kMaxColumns = 240;  // 240*8 = 1920px
  static constexpr size_t kMaxRows = 100;     // 100*16 = 1600px
  static constexpr size_t kScrollbackLines = 200;

  Terminal() = default;
  ~Terminal() = default;
  Terminal(const Terminal&) = delete;
  Terminal& operator=(const Terminal&) = delete;

  // Initialise the framebuffer terminal. Must be called after
  // Framebuffer::init(). Returns false if no framebuffer is available.
  bool init();

  [[nodiscard]] bool is_active() const { return active_; }
  [[nodiscard]] size_t rows() const { return rows_; }
  [[nodiscard]] size_t cols() const { return cols_; }
  [[nodiscard]] bool is_scrolled_back() const { return scroll_offset_ > 0; }

  void write(const char* data);
  void write(std::span<const char> data);
  void write_char(char c);
  void put_char(char c);
  void handle_key(KeyEvent event);
  void set_color(uint8_t color);
  void clear();
  void set_position(size_t row, size_t col);

  // Scroll the visible window by delta rows (positive = up, negative = down).
  // Clamped to [0, kScrollbackLines]. Redraws the framebuffer.
  void scroll_view(int delta);

  // Test accessors for reading cell contents.
  [[nodiscard]] char cell_char(size_t row, size_t col) const { return cells_[row][col].ch; }
  [[nodiscard]] uint8_t cell_color(size_t row, size_t col) const { return cells_[row][col].color; }

 private:
  // Draw glyph for character c at grid position (row, col) with given color.
  void draw_glyph(char c, uint8_t color, size_t row, size_t col);

  // Draw the cursor block at the current position.
  void draw_cursor();

  // Erase the cursor block (redraw the character underneath).
  void erase_cursor();

  void scroll();
  void clear_line(size_t row);
  void dispatch_csi(char final_byte);
  void apply_sgr(uint8_t nparams);
  void redraw_all();

  // Convert a 4-bit color index to a 32-bit XRGB framebuffer color.
  [[nodiscard]] static uint32_t color_to_rgb(uint8_t color_index);

  enum class EscState : uint8_t { Normal, Escape, Csi };
  static constexpr size_t kMaxCsiParams = 8;

  struct Cursor {
    size_t row = 0;
    size_t col = 0;
  };

  bool active_ = false;
  bool cursor_visible_ = true;

  Cursor cursor_{};
  uint8_t color_ = 0x07;  // light grey on black

  // Character buffer for scrollback (stores char + color for redraw).
  struct Cell {
    char ch = 0;
    uint8_t color = 0x07;
  };

  // Actual usable dimensions, computed from the framebuffer in init().
  size_t rows_ = 0;
  size_t cols_ = 0;

  // Live visible content (rows 0..rows_-1 = top..bottom of current screen).
  Cell cells_[kMaxRows][kMaxColumns]{};

  // Ring buffer of lines scrolled off the top. scrollback_head_ points to the
  // next write slot; lines_scrolled_ tracks how many valid entries there are
  // (capped at kScrollbackLines).
  Cell scrollback_[kScrollbackLines][kMaxColumns]{};
  size_t scrollback_head_ = 0;
  size_t lines_scrolled_ = 0;
  // How many rows we are currently scrolled back from the live view (0=live).
  size_t scroll_offset_ = 0;

  EscState esc_state_ = EscState::Normal;
  bool csi_private_ = false;
  std::array<uint16_t, kMaxCsiParams> esc_params_{};
  uint8_t esc_param_count_ = 0;

  // Framebuffer geometry (cached from Framebuffer::info()).
  uint8_t* fb_ = nullptr;
  uint32_t pitch_ = 0;
  uint32_t bpp_ = 0;
};

extern Terminal kTerminal;

#pragma once

#include <array.h>
#include <span.h>
#include <stddef.h>
#include <stdint.h>

#include "keyboard.h"

// Framebuffer-based text console.
//
// Renders an 80x25 character grid using an 8x16 bitmap font onto the
// linear framebuffer.
class Terminal {
 public:
  static constexpr size_t kFontWidth = 8;
  static constexpr size_t kFontHeight = 16;
  static constexpr size_t kColumns = 80;
  static constexpr size_t kRows = 25;

  Terminal() = default;
  ~Terminal() = default;
  Terminal(const Terminal&) = delete;
  Terminal& operator=(const Terminal&) = delete;

  // Initialise the framebuffer terminal. Must be called after
  // Framebuffer::init(). Returns false if no framebuffer is available.
  bool init();

  [[nodiscard]] bool is_active() const { return active_; }

  void write(const char* data);
  void write(std::span<const char> data);
  void write_char(char c);
  void put_char(char c);
  void handle_key(KeyEvent event);
  void set_color(uint8_t color);
  void clear();
  void set_position(size_t row, size_t col);

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

  // Convert a 4-bit color index to a 32-bit XRGB framebuffer color.
  [[nodiscard]] static uint32_t color_to_rgb(uint8_t color_index);

  enum class EscState : uint8_t { Normal, Escape, Csi };
  static constexpr size_t kMaxCsiParams = 8;

  bool active_ = false;
  bool cursor_visible_ = true;

  size_t row_ = 0;
  size_t col_ = 0;
  uint8_t color_ = 0x07;  // light grey on black

  // Character buffer for scrollback (stores char + color for redraw).
  struct Cell {
    char ch = 0;
    uint8_t color = 0x07;
  };
  Cell cells_[kRows][kColumns]{};

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

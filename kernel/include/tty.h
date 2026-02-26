#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

void terminal_write(const char* data);
void terminal_putchar(char c);
void terminal_clear(void);
void terminal_set_position(unsigned int row, unsigned int col);
void terminal_set_color(unsigned char color);

__END_DECLS

#ifdef __cplusplus

#include <array.h>

#include "keyboard.h"
#include "vga.h"

class Terminal {
 public:
  Terminal();
  ~Terminal() = default;
  Terminal(const Terminal&) = delete;
  Terminal& operator=(const Terminal&) = delete;

  void write(const char* data);
  void put_char(char c);
  void handle_key(KeyEvent event);
  void set_color(uint8_t color);
  void clear();
  void set_position(size_t row, size_t col);
  void enable_cursor();
  void disable_cursor();
  void update_cursor();

 private:
  void put_entry_at(char c, uint8_t color, size_t row, size_t col);
  void scroll();
  void clear_line(size_t row);
  void move_cursor(size_t row, size_t col);
  void dispatch_csi(char final_byte);
  void apply_sgr(uint8_t nparams);

  enum class EscState : uint8_t { Normal, Escape, Csi };
  static constexpr size_t kMaxCsiParams = 8;

  bool cursor_enabled_ = false;

  size_t row_ = 0;
  size_t col_ = 0;
  uint8_t color_ = VGA::entry_color(VGA::Color::LightGrey, VGA::Color::Black);
  uint8_t cursor_color_ = VGA::entry_color(VGA::Color::Black, VGA::Color::LightGrey);
  volatile uint16_t* buffer_ = VGA::MEMORY_ADDR;

  EscState esc_state_;
  std::array<uint16_t, kMaxCsiParams> esc_params_{};
  uint8_t esc_param_count_;
};

extern Terminal kTerminal;

#endif /* __cplusplus */

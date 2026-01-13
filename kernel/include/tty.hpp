#pragma once

#include <stddef.h>
#include <stdint.h>

#include "keyboard.hpp"
#include "vga.hpp"

class Terminal {
   public:
    Terminal();
    ~Terminal() = default;
    Terminal(const Terminal &) = delete;
    Terminal &operator=(const Terminal &) = delete;

    void write(const char *data);
    void put_char(char c);
    void handle_key(KeyEvent event);
    void set_color(uint8_t color);
    void enable_cursor();
    void disable_cursor();
    void update_cursor();

   private:
    void put_entry_at(char c, uint8_t color, size_t row, size_t col);
    void scroll();
    void clear_line(size_t row);
    void move_cursor(size_t row, size_t col);

    bool cursor_enabled_ = false;

    size_t row_ = 0;
    size_t col_ = 0;
    uint8_t color_ = VGA::entry_color(VGA::Color::LightGrey, VGA::Color::Black);
    uint8_t cursor_color_ =
        VGA::entry_color(VGA::Color::Black, VGA::Color::LightGrey);
    volatile uint16_t *buffer_ = VGA::MEMORY_ADDR;
};

extern Terminal kTerminal;

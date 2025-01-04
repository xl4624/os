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

    void write_string(const char *data);
    void write(const char *data, size_t size);
    void put_char(char c);
    void handle_key(KeyEvent event);
    void set_color(uint8_t color);

   private:
    struct Position {
        size_t row;
        size_t col;

        void advance();

        void newline();
        bool backspace();
        bool move_up();
        bool move_down();
        void move_left();
        // TODO: should not move further if it hits a '0'
        void move_right();
        size_t to_index() const;
    };

    // screen operations
    void put_entry_at(char c, uint8_t color, Position pos);
    void scroll();
    void clear_line(size_t row);

    // character handling
    void handle_newline();
    void handle_backspace();
    void handle_tab();
    void handle_char(char c);

    // cursor operations
    void update_cursor();
    void clear_cursor();

    size_t find_last_char_in_row(size_t row) const;

    Position pos_ = {.row = 0, .col = 0};
    uint8_t color_ = VGA::entry_color(VGA::Color::LightGrey, VGA::Color::Black);
    uint8_t cursor_color_ =
        VGA::entry_color(VGA::Color::Black, VGA::Color::LightGrey);
    volatile uint16_t *buffer_ = VGA::MEMORY;
};

extern Terminal terminal;

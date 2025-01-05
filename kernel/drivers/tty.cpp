#include "tty.hpp"

#include <string.h>
#include <sys/param.h>

Terminal terminal;

Terminal::Terminal() {
    for (size_t y = 0; y < VGA::HEIGHT; y++) {
        clear_line(y);
    }
}

void Terminal::write_string(const char *data) {
    write(data, strlen(data));
}

void Terminal::write(const char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        put_char(data[i]);
    }
}

void Terminal::put_char(char c) {
    clear_cursor();
    switch (c) {
        case '\n': handle_newline(); break;
        case '\b': handle_backspace(); break;
        case '\t': handle_tab(); break;
        default: handle_char(c); break;
    }
    update_cursor();
}

void Terminal::handle_newline() {
    pos_.newline();
    if (pos_.row == VGA::HEIGHT - 1) {
        scroll();
    }
}

void Terminal::handle_backspace() {
    if (pos_.backspace()) {
        const size_t last_char = find_last_char_in_row(pos_.row);
        if (pos_.col > last_char) {
            pos_.col = last_char;
        }
        put_entry_at(0, color_, pos_);
    }
}

void Terminal::handle_tab() {
    size_t spaces = 4 - (pos_.col % 4);
    for (size_t i = 0; i < spaces; i++) {
        handle_char(' ');
    }
}

void Terminal::handle_char(char c) {
    put_entry_at(c, color_, pos_);
    pos_.advance();
    if (pos_.row == VGA::HEIGHT - 1 && pos_.col == 0) {
        scroll();
    }
}

void Terminal::handle_key(KeyEvent event) {
    if (!event.pressed)
        return;

    if (event.ascii) {
        put_char(event.ascii);
        return;
    }

    clear_cursor();
    switch (event.key) {
        case Key::Up: pos_.move_up(); break;
        case Key::Down: pos_.move_down(); break;
        case Key::Left: pos_.move_left(); break;
        case Key::Right: pos_.move_right(); break;
        default: break;
    }
    update_cursor();
}

void Terminal::put_entry_at(char c, uint8_t color, Position pos) {
    buffer_[pos.to_index()] = VGA::entry(c, color);
}

void Terminal::scroll() {
    clear_cursor();

    for (size_t i = 1; i < VGA::HEIGHT; i++) {
        memmove(const_cast<uint16_t *>(&buffer_[(i - 1) * VGA::WIDTH]),
                const_cast<const uint16_t *>(&buffer_[i * VGA::WIDTH]),
                VGA::WIDTH * sizeof(uint16_t));
    }

    // clear bottom line
    clear_line(VGA::HEIGHT - 1);
    pos_.row--;
}

void Terminal::clear_line(size_t row) {
    for (size_t col = 0; col < VGA::WIDTH; col++) {
        put_entry_at(0, color_, Position{row, col});
    }
}

void Terminal::update_cursor() {
    const size_t index = pos_.to_index();
    char c = buffer_[index] & 0xFF;
    buffer_[index] = VGA::entry(c, cursor_color_);
}

void Terminal::clear_cursor() {
    const size_t index = pos_.to_index();
    char c = buffer_[index] & 0xFF;
    buffer_[index] = VGA::entry(c, color_);
}

size_t Terminal::find_last_char_in_row(size_t row) const {
    size_t col = VGA::WIDTH - 1;
    while (col > 0 && (buffer_[row * VGA::WIDTH + col - 1] & 0xFF) == 0) {
        col--;
    }
    return col;
}

void Terminal::set_color(uint8_t color) {
    color_ = color;
}

// =====================
//      C Interface
// =====================

extern "C" {

void terminal_writestring(const char *data) {
    terminal.write_string(data);
}

void terminal_write(const char *data, size_t size) {
    terminal.write(data, size);
}

void terminal_putchar(char c) {
    terminal.put_char(c);
}
}

// =========================
//      Position Struct
// =========================

void Terminal::Position::advance() {
    if (++col == VGA::WIDTH) {
        col = 0;
        if (++row == VGA::HEIGHT) {
            row = VGA::HEIGHT - 1;
        }
    }
}

void Terminal::Position::newline() {
    col = 0;
    if (++row == VGA::HEIGHT) {
        row = VGA::HEIGHT - 1;
    }
}

bool Terminal::Position::backspace() {
    if (col > 0) {
        col--;
        return true;
    }
    if (row > 0) {
        row--;
        col = VGA::WIDTH - 1;
        return true;
    }
    return false;
}

void Terminal::Position::move_up() {
    if (row > 0) {
        row--;
    }
    size_t last_char = terminal.find_last_char_in_row(row);
    col = MIN(col, last_char);
}

void Terminal::Position::move_down() {
    if (row < VGA::HEIGHT - 1) {
        row++;
    }
    size_t last_char = terminal.find_last_char_in_row(row);
    col = MIN(col, last_char);
}

void Terminal::Position::move_left() {
    if (col > 0) {
        col--;
    } else if (row > 0) {
        row--;
        col = VGA::WIDTH - 1;
    }
    size_t last_char = terminal.find_last_char_in_row(row);
    col = MIN(col, last_char);
}

void Terminal::Position::move_right() {
    size_t last_char = terminal.find_last_char_in_row(row);
    if (col < last_char) {
        col++;
    } else if (row < VGA::HEIGHT - 1) {
        row++;
        col = 0;
    }
}

size_t Terminal::Position::to_index() const {
    return row * VGA::WIDTH + col;
}

#include "tty.hpp"

#include <string.h>

#include "vga.hpp"

Terminal terminal;

Terminal::Terminal()
    : row_(0),
      column_(0),
      color_(VGA::entryColor(VGA::Color::LightGrey, VGA::Color::Black)),
      buffer_(VGA::MEMORY) {
    for (size_t y = 0; y < VGA::HEIGHT; y++) {
        for (size_t x = 0; x < VGA::WIDTH; x++) {
            putEntryAt(' ', color_, x, y);
        }
    }
}

void Terminal::writeString(const char *data) {
    write(data, strlen(data));
}

void Terminal::write(const char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        putChar(data[i]);
    }
}

// TODO: Handle other special characters like '\b' or '\t'
void Terminal::putChar(char c) {
    if (c == '\n') {
        column_ = 0;
        if (++row_ == VGA::HEIGHT) {
            scroll();
        }
    } else if (c == '\b') {
        if (column_ > 0) {
            column_--;
            putEntryAt(' ', color_, column_, row_);
        } else if (row_ > 0) {
            row_--;
            column_ = VGA::WIDTH - 1;
            while (column_ > 0 && (buffer_[row_ * VGA::WIDTH + column_ - 1] & 0xFF) == ' ') {
                column_--;
            }
            putEntryAt(' ', color_, column_, row_);
        }
    } else {
        putEntryAt(c, color_, column_, row_);
        if (++column_ == VGA::WIDTH) {
            column_ = 0;
            if (++row_ == VGA::HEIGHT) {
                scroll();
            }
        }
    }
}

void Terminal::setColor(uint8_t color) {
    color_ = color;
}

void Terminal::putEntryAt(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA::WIDTH + x;
    buffer_[index] = VGA::entry(c, color);
}

void Terminal::scroll() {
    for (size_t i = 1; i < VGA::HEIGHT; i++) {
        for (size_t j = 0; j < VGA::WIDTH; j++) {
            const size_t row1_ind = (i - 1) * VGA::WIDTH + j;
            const size_t row2_ind = (i)*VGA::WIDTH + j;
            buffer_[row1_ind] = buffer_[row2_ind];
        }
    }
    row_--;
    for (size_t col = 0; col < VGA::WIDTH; col++) {
        putEntryAt(' ', color_, col, row_);
    }
}

// ===============================
//           C Interface
// ===============================

extern "C" {

void terminal_writestring(const char *data) {
    terminal.writeString(data);
}

void terminal_write(const char *data, size_t size) {
    terminal.write(data, size);
}

void terminal_putchar(char c) {
    terminal.putChar(c);
}
}

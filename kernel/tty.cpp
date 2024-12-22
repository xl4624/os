#include "tty.hpp"

#include "vga.hpp"

// TODO: Move this to part of the standard library
size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len])
        len++;
    return len;
}

Terminal::Terminal() {
    row_ = 0;
    column_ = 0;
    color_ = VGA::entryColor(VGA::Color::LIGHT_GREY, VGA::Color::BLACK);
    buffer_ = (uint16_t *)VGA::MEMORY_START;
    for (size_t y = 0; y < VGA::HEIGHT; y++) {
        for (size_t x = 0; x < VGA::WIDTH; x++) {
            putEntryAt(' ', color_, x, y);
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

void Terminal::putChar(char c) {
    if (c == '\n') {
        column_ = 0;
        if (++row_ == VGA::HEIGHT) {
            row_ = 0;
        }
    } else {
        putEntryAt(c, color_, column_, row_);
        if (++column_ == VGA::WIDTH) {
            column_ = 0;
            if (++row_ == VGA::HEIGHT) {
                row_ = 0;
            }
        }
    }
}

void Terminal::write(const char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        putChar(data[i]);
    }
}

void Terminal::writeString(const char *data) {
    write(data, strlen(data));
}

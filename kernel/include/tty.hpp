#pragma once

#include <stddef.h>
#include <stdint.h>

class Terminal {
   public:
    static Terminal &getInstance();
    void putChar(char c);
    void write(const char *data, size_t size);
    void writeString(const char *data);

   private:
    Terminal();

    Terminal(const Terminal &) = delete;
    Terminal &operator=(const Terminal &) = delete;
    void setColor(uint8_t color);
    void putEntryAt(char c, uint8_t color, size_t x, size_t y);
    void scroll();

    size_t row_;
    size_t column_;
    uint8_t color_;
    uint16_t *buffer_;
};

#pragma once

#include <stddef.h>
#include <stdint.h>

class Terminal {
   public:
    Terminal();
    Terminal(const Terminal &) = delete;
    Terminal &operator=(const Terminal &) = delete;

    void writeString(const char *data);
    void write(const char *data, size_t size);
    void putChar(char c);
    void setColor(uint8_t color);

   private:
    void putEntryAt(char c, uint8_t color, size_t x, size_t y);
    void scroll();

    size_t row_;
    size_t column_;
    uint8_t color_;
    volatile uint16_t *buffer_;
};

extern Terminal terminal;

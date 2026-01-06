#pragma once

#include <stddef.h>
#include <stdint.h>

namespace VGA {
    constexpr size_t WIDTH = 80;
    constexpr size_t HEIGHT = 25;

    inline uint16_t *MEMORY = reinterpret_cast<uint16_t *>(0xC00B8000);

    /* Hardware text mode color constants. */
    enum class Color : uint8_t {
        Black = 0,
        Blue = 1,
        Green = 2,
        Cyan = 3,
        Red = 4,
        Magenta = 5,
        Brown = 6,
        LightGrey = 7,
        DarkGrey = 8,
        LightBlue = 9,
        LightGreen = 10,
        LightCyan = 11,
        LightRed = 12,
        LightMagenta = 13,
        LightBrown = 14,
        White = 15,
    };

    constexpr uint8_t entry_color(Color fg, Color bg) {
        return static_cast<uint8_t>(fg) | (static_cast<uint8_t>(bg) << 4);
    }

    constexpr uint16_t entry(char c, uint8_t color) {
        return static_cast<uint16_t>(c) | (static_cast<uint16_t>(color) << 8);
    }
}  // namespace VGA

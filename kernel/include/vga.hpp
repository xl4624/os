#pragma once

#include <stddef.h>
#include <stdint.h>

namespace VGA {
    constexpr size_t WIDTH = 80;
    constexpr size_t HEIGHT = 25;

    inline uint16_t *MEMORY_ADDR = reinterpret_cast<uint16_t *>(0xC00B8000);

    constexpr uint16_t CRTC_ADDR_REG = 0x3D4;
    constexpr uint16_t CRTC_DATA_REG = 0x3D5;

    constexpr uint8_t CURSOR_START_REG = 0x0A;
    constexpr uint8_t CURSOR_END_REG = 0x0B;
    constexpr uint8_t CURSOR_LOC_LOW_REG = 0x0F;
    constexpr uint8_t CURSOR_LOC_HIGH_REG = 0x0E;

    constexpr uint8_t CURSOR_DISABLE_BIT = 0x20;
    constexpr uint8_t DEFAULT_CURSOR_START = 14;
    constexpr uint8_t DEFAULT_CURSOR_END = 15;
    // 1100 0000 - preserve bits 7-6
    constexpr uint8_t CURSOR_START_MASK = 0xC0;
    // 1110 0000 - preserve bits 7-5
    constexpr uint8_t CURSOR_END_MASK = 0xE0;

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

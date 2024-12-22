#pragma once

#include <stddef.h>
#include <stdint.h>

namespace VGA {
    constexpr size_t WIDTH = 80;
    constexpr size_t HEIGHT = 25;

    const uint16_t *MEMORY_START = reinterpret_cast<uint16_t *>(0xB8000);

    /* Hardware text mode color constants. */
    enum class Color : uint8_t {
        BLACK = 0,
        BLUE = 1,
        GREEN = 2,
        CYAN = 3,
        RED = 4,
        MAGENTA = 5,
        BROWN = 6,
        LIGHT_GREY = 7,
        DARK_GREY = 8,
        LIGHT_BLUE = 9,
        LIGHT_GREEN = 10,
        LIGHT_CYAN = 11,
        LIGHT_RED = 12,
        LIGHT_MAGENTA = 13,
        LIGHT_BROWN = 14,
        WHITE = 15,
    };

    inline uint8_t entryColor(Color fg, Color bg) {
        return static_cast<uint8_t>(fg) | (static_cast<uint8_t>(bg) << 4);
    }

    inline uint16_t entry(char c, uint8_t color) {
        return static_cast<uint16_t>(c) | (static_cast<uint16_t>(color) << 8);
    }
}  // namespace VGA

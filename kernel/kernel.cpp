#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__) || !defined(__i386__)
    #error "ix86-elf cross compiler required"
#endif

namespace VGA {
    const size_t WIDTH = 80;
    const size_t HEIGHT = 25;

    const uint16_t *MEMORY_START = (uint16_t *)0xB8000;

    /* Hardware text mode color constants. */
    enum class Color : uint8_t {
        // clang-format off
        BLACK         = 0,
        BLUE          = 1,
        GREEN         = 2,
        CYAN          = 3,
        RED           = 4,
        MAGENTA       = 5,
        BROWN         = 6,
        LIGHT_GREY    = 7,
        DARK_GREY     = 8,
        LIGHT_BLUE    = 9,
        LIGHT_GREEN   = 10,
        LIGHT_CYAN    = 11,
        LIGHT_RED     = 12,
        LIGHT_MAGENTA = 13,
        LIGHT_BROWN   = 14,
        WHITE         = 15,
        // clang-format on
    };

    inline uint8_t entryColor(Color fg, Color bg) {
        return (static_cast<uint8_t>(fg) | (static_cast<uint8_t>(bg) << 4));
    }

    inline uint16_t entry(char c, uint8_t color) {
        return static_cast<uint16_t>(c) | (static_cast<uint16_t>(color) << 8);
    }
};  // namespace VGA

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len])
        len++;
    return len;
}

class Terminal {
   public:
    Terminal() {
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

    void setColor(uint8_t color) {
        color_ = color;
    }

    void putEntryAt(char c, uint8_t color, size_t x, size_t y) {
        const size_t index = y * VGA::WIDTH + x;
        buffer_[index] = VGA::entry(c, color);
    }

    void putChar(char c) {
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

    void write(const char *data, size_t size) {
        for (size_t i = 0; i < size; i++) {
            putChar(data[i]);
        }
    }

    void writeString(const char *data) {
        write(data, strlen(data));
    }

   private:
    size_t row_;
    size_t column_;
    uint8_t color_;
    uint16_t *buffer_;
};

extern "C" void kernel_main(void) {
    Terminal terminal;

    terminal.writeString("Hello world!\n");
}

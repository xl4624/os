#include "tty.hpp"

#include <stdio.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/io.h>
#include <sys/param.h>

constexpr size_t TAB_WIDTH = 4;

[[nodiscard]]
static constexpr size_t to_index(size_t row, size_t col) {
    return row * VGA::WIDTH + col;
}

// The global VGA text-mode terminal instance.
Terminal kTerminal;

Terminal::Terminal() {
    for (size_t y = 0; y < VGA::HEIGHT; ++y)
        clear_line(y);
    enable_cursor();
}

void Terminal::write(const char *data) {
    while (*data)
        put_char(*data++);
}

void Terminal::put_char(char c) {
    if (c == '\n') {
        col_ = 0;
        if (++row_ == VGA::HEIGHT) {
            scroll();
        }
    } else if (c == '\b') {
        if (col_ > 0) {
            --col_;
        } else if (row_ > 0) {
            --row_;
            size_t tmp = VGA::WIDTH - 1;
            while (tmp > 0 && (buffer_[to_index(row_, tmp - 1)] & 0xFF) == 0) {
                --tmp;
            }
            col_ = tmp;
        }
        put_entry_at(0, color_, row_, col_);
        update_cursor();
    } else if (c == '\t') {
        for (size_t i = 0; i < TAB_WIDTH - (col_ % TAB_WIDTH); ++i) {
            put_char(' ');
        }
    } else {
        put_entry_at(c, color_, row_, col_);
        if (++col_ == VGA::WIDTH) {
            col_ = 0;
            if (++row_ == VGA::HEIGHT) {
                scroll();
            }
        }
    }
    update_cursor();
}

void Terminal::handle_key(KeyEvent event) {
    if (!event.pressed) {
        return;
    }

    if (event.ascii) {
        put_char(event.ascii);
        return;
    }

    switch (event.key.value()) {
        case Key::Up:
            if (row_ > 0) {
                --row_;
                update_cursor();
            }
            break;
        case Key::Down:
            if (row_ < VGA::HEIGHT - 1) {
                ++row_;
                update_cursor();
            }
            break;
        case Key::Left:
            if (col_ > 0) {
                --col_;
                update_cursor();
            } else if (row_ > 0) {
                --row_;
                col_ = VGA::WIDTH - 1;
                update_cursor();
            }
            break;
        case Key::Right:
            if (col_ < VGA::WIDTH - 1) {
                ++col_;
                update_cursor();
            } else if (row_ < VGA::HEIGHT - 1) {
                ++row_;
                col_ = 0;
                update_cursor();
            }
            break;
        default: break;
    }
}

void Terminal::put_entry_at(char c, uint8_t color, size_t row, size_t col) {
    buffer_[to_index(row, col)] = VGA::entry(c, color);
}

void Terminal::scroll() {
    for (size_t r = 1; r < VGA::HEIGHT; ++r) {
        for (size_t c = 0; c < VGA::WIDTH; ++c) {
            buffer_[to_index((r - 1), c)] = buffer_[to_index(r, c)];
        }
    }

    // clear bottom line
    clear_line(VGA::HEIGHT - 1);
    row_--;
    update_cursor();
}

void Terminal::clear_line(size_t row) {
    for (size_t col = 0; col < VGA::WIDTH; ++col) {
        put_entry_at(0, color_, row, col);
    }
}

void Terminal::set_color(uint8_t color) {
    color_ = color;
}

void Terminal::enable_cursor() {
    cursor_enabled_ = true;
    outb(VGA::CRTC_ADDR_REG, VGA::CURSOR_START_REG);
    outb(VGA::CRTC_DATA_REG, (inb(VGA::CRTC_DATA_REG) & VGA::CURSOR_START_MASK)
                                 | VGA::DEFAULT_CURSOR_START);
    outb(VGA::CRTC_ADDR_REG, VGA::CURSOR_END_REG);
    outb(VGA::CRTC_DATA_REG, (inb(VGA::CRTC_DATA_REG) & VGA::CURSOR_END_MASK)
                                 | VGA::DEFAULT_CURSOR_END);
}

void Terminal::disable_cursor() {
    cursor_enabled_ = false;
    outb(VGA::CRTC_ADDR_REG, VGA::CURSOR_START_REG);
    outb(VGA::CRTC_DATA_REG, VGA::CURSOR_DISABLE_BIT);
}

void Terminal::update_cursor() {
    if (!cursor_enabled_)
        return;
    move_cursor(row_, col_);
}

void Terminal::move_cursor(size_t row, size_t col) {
    uint16_t pos = static_cast<uint16_t>(row * VGA::WIDTH + col);
    outb(VGA::CRTC_ADDR_REG, VGA::CURSOR_LOC_LOW_REG);
    outb(VGA::CRTC_DATA_REG, static_cast<uint8_t>(pos & 0xFF));
    outb(VGA::CRTC_ADDR_REG, VGA::CURSOR_LOC_HIGH_REG);
    outb(VGA::CRTC_DATA_REG, static_cast<uint8_t>((pos >> 8) & 0xFF));
}

// =====================
//      C Interface
// =====================

__BEGIN_DECLS
void terminal_write(const char *data) {
    kTerminal.write(data);
}

void terminal_putchar(char c) {
    kTerminal.put_char(c);
}
__END_DECLS

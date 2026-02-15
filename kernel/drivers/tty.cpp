#include "tty.hpp"

#include <stdio.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/param.h>

constexpr size_t TAB_WIDTH = 4;

[[nodiscard]]
static constexpr size_t to_index(size_t row, size_t col) {
    return row * VGA::WIDTH + col;
}

Terminal terminal;

Terminal::Terminal() {
    for (size_t y = 0; y < VGA::HEIGHT; ++y)
        clear_line(y);
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
}

void Terminal::handle_key(KeyEvent event) {
    if (!event.pressed)
        return;

    if (event.ascii) {
        put_char(event.ascii);
        return;
    }

    switch (event.key) {
        // TODO: Handle when enabling cursors
        case Key::Up: printf("KEY UP PRESSED\n"); break;
        case Key::Down: printf("KEY DOWN PRESSED\n"); break;
        case Key::Left: printf("KEY LEFT PRESSED\n"); break;
        case Key::Right: printf("KEY RIGHT PRESSED\n"); break;
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
}

void Terminal::clear_line(size_t row) {
    for (size_t col = 0; col < VGA::WIDTH; ++col) {
        put_entry_at(0, color_, row, col);
    }
}

void Terminal::set_color(uint8_t color) {
    color_ = color;
}

// =====================
//      C Interface
// =====================

__BEGIN_DECLS
void terminal_write(const char *data) {
    terminal.write(data);
}

void terminal_putchar(char c) {
    terminal.put_char(c);
}
__END_DECLS

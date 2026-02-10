#include "ktest.h"
#include "tty.h"
#include "vga.h"

// Helpers to read the VGA text buffer directly. kTerminal's buffer_ points
// to VGA::MEMORY_ADDR (0xC00B8000) so reads below reflect what was written.

static char vga_char_at(size_t row, size_t col) {
    return static_cast<char>(VGA::MEMORY_ADDR[row * VGA::WIDTH + col] & 0xFF);
}

static uint8_t vga_color_at(size_t row, size_t col) {
    return static_cast<uint8_t>(VGA::MEMORY_ADDR[row * VGA::WIDTH + col] >> 8);
}

// ---------------------------------------------------------------------------
// Character output
// ---------------------------------------------------------------------------

TEST(tty, putchar_appears_in_buffer) {
    terminal_clear();
    terminal_putchar('A');
    // terminal_clear() resets cursor to (0, 0). 'A' lands there.
    ASSERT_EQ(vga_char_at(0, 0), 'A');
}

TEST(tty, multiple_chars_fill_columns) {
    terminal_clear();
    terminal_putchar('H');
    terminal_putchar('i');
    ASSERT_EQ(vga_char_at(0, 0), 'H');
    ASSERT_EQ(vga_char_at(0, 1), 'i');
}

// ---------------------------------------------------------------------------
// Newline
// ---------------------------------------------------------------------------

TEST(tty, newline_advances_row) {
    terminal_clear();
    terminal_putchar('\n');
    terminal_putchar('X');
    // '\n' resets col to 0 and increments row to 1.
    ASSERT_EQ(vga_char_at(1, 0), 'X');
}

// ---------------------------------------------------------------------------
// Backspace
// ---------------------------------------------------------------------------

TEST(tty, backspace_erases_last_char) {
    terminal_clear();
    terminal_putchar('A');
    terminal_putchar('\b');
    // Backspace decrements col and writes char 0 at the vacated cell.
    ASSERT_EQ(vga_char_at(0, 0), 0);
}

// ---------------------------------------------------------------------------
// Tab
// ---------------------------------------------------------------------------

TEST(tty, tab_advances_cursor) {
    terminal_clear();
    // Tab from col 0 writes spaces and advances the cursor past col 0.
    // Verify the next character lands after the spaces.
    terminal_putchar('\t');
    size_t tab_col = 0;
    // Find where the cursor ended up by looking for the first non-space
    // after we write a marker.
    terminal_putchar('T');
    for (size_t c = 0; c < VGA::WIDTH; ++c) {
        if (vga_char_at(0, c) == 'T') {
            tab_col = c;
            break;
        }
    }
    // Tab should advance past col 0.
    ASSERT_TRUE(tab_col > 0);
    ASSERT_EQ(vga_char_at(0, tab_col), 'T');
}

// ---------------------------------------------------------------------------
// Color
// ---------------------------------------------------------------------------

TEST(tty, set_color_changes_attribute) {
    terminal_clear();
    const uint8_t color =
        VGA::entry_color(VGA::Color::Red, VGA::Color::Blue);
    terminal_set_color(color);
    terminal_putchar('Z');
    ASSERT_EQ(vga_char_at(0, 0), 'Z');
    ASSERT_EQ(vga_color_at(0, 0), color);
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

TEST(tty, clear_resets_to_origin) {
    terminal_clear();
    terminal_putchar('A');
    terminal_putchar('B');
    terminal_clear();
    // After clear, cursor is at (0, 0) again.
    terminal_putchar('Q');
    ASSERT_EQ(vga_char_at(0, 0), 'Q');
}

// ---------------------------------------------------------------------------
// set_position
// ---------------------------------------------------------------------------

TEST(tty, set_position_moves_cursor) {
    terminal_clear();
    terminal_set_position(3, 5);
    terminal_putchar('P');
    ASSERT_EQ(vga_char_at(3, 5), 'P');
}

TEST(tty, set_position_clamps_out_of_range) {
    terminal_clear();
    // Row >= HEIGHT is clamped to HEIGHT-1. Use col=0 so writing 'C' does
    // not overflow to the next line and trigger a scroll.
    terminal_set_position(999, 0);
    terminal_putchar('C');
    ASSERT_EQ(vga_char_at(VGA::HEIGHT - 1, 0), 'C');
}

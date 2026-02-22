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
// terminal_putchar
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

TEST(tty, newline_advances_row) {
    terminal_clear();
    terminal_putchar('\n');
    terminal_putchar('X');
    // '\n' resets col to 0 and increments row to 1.
    ASSERT_EQ(vga_char_at(1, 0), 'X');
}

TEST(tty, backspace_erases_last_char) {
    terminal_clear();
    terminal_putchar('A');
    terminal_putchar('\b');
    // Backspace decrements col and writes char 0 at the vacated cell.
    ASSERT_EQ(vga_char_at(0, 0), 0);
}

// A tab advances the cursor to the next TAB_WIDTH (4) boundary.
// From col 0: 4 spaces -> cursor lands at col 4.
// From col 1: 3 spaces -> cursor lands at col 4.
// From col 4: 4 spaces -> cursor lands at col 8.
TEST(tty, tab_advances_to_next_tab_stop) {
    terminal_clear();
    terminal_putchar('\t');
    terminal_putchar('A');
    ASSERT_EQ(vga_char_at(0, 4), 'A');

    // From col 1: one char then tab should land at col 4.
    terminal_clear();
    terminal_putchar('X');   // col 1
    terminal_putchar('\t');  // 3 spaces -> col 4
    terminal_putchar('B');
    ASSERT_EQ(vga_char_at(0, 4), 'B');

    // From col 4: tab should land at col 8.
    terminal_clear();
    terminal_set_position(0, 4);
    terminal_putchar('\t');
    terminal_putchar('C');
    ASSERT_EQ(vga_char_at(0, 8), 'C');
}

// ---------------------------------------------------------------------------
// terminal_set_color
// ---------------------------------------------------------------------------

TEST(tty, set_color_changes_attribute) {
    terminal_clear();
    const uint8_t color = VGA::entry_color(VGA::Color::Red, VGA::Color::Blue);
    terminal_set_color(color);
    terminal_putchar('Z');
    ASSERT_EQ(vga_char_at(0, 0), 'Z');
    ASSERT_EQ(vga_color_at(0, 0), color);
}

// ---------------------------------------------------------------------------
// terminal_clear
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
// terminal_set_position
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

TEST(tty, set_position_col_clamps_out_of_range) {
    terminal_clear();
    // Col >= WIDTH is clamped to WIDTH-1.
    terminal_set_position(0, 999);
    terminal_putchar('D');
    ASSERT_EQ(vga_char_at(0, VGA::WIDTH - 1), 'D');
}

TEST(tty, line_wrap_at_right_edge) {
    terminal_clear();
    // Write exactly WIDTH characters to fill the first row.
    for (size_t i = 0; i < VGA::WIDTH; ++i) {
        terminal_putchar('A');
    }
    // The next character must wrap to the start of row 1.
    terminal_putchar('B');
    ASSERT_EQ(vga_char_at(1, 0), 'B');
}

TEST(tty, scroll_on_overflow) {
    terminal_clear();
    // Leave a marker at row 0 col 0.
    terminal_putchar('S');
    // VGA::HEIGHT newlines from row 0 advance through every row and trigger
    // one scroll, which shifts all rows up by one and clears the last row.
    for (size_t i = 0; i < VGA::HEIGHT; ++i) {
        terminal_putchar('\n');
    }
    // The 'S' that was at row 0 should have scrolled off; row 0 is now blank.
    ASSERT_EQ(vga_char_at(0, 0), 0);
}

TEST(tty, color_persists_across_newline) {
    terminal_clear();
    const uint8_t color =
        VGA::entry_color(VGA::Color::Green, VGA::Color::Black);
    terminal_set_color(color);
    terminal_putchar('A');
    terminal_putchar('\n');
    terminal_putchar('B');
    // Both characters must carry the color set before the newline.
    ASSERT_EQ(vga_color_at(0, 0), color);
    ASSERT_EQ(vga_color_at(1, 0), color);
}

TEST(tty, backspace_at_col_zero_goes_to_previous_row) {
    terminal_clear();
    terminal_putchar('\n');  // advance to row 1, col 0
    // Backspace at col 0 of a non-zero row should retreat to the previous row.
    // The previous row (0) is blank, so the cursor lands at (0, 0).
    terminal_putchar('\b');
    terminal_putchar('R');
    ASSERT_EQ(vga_char_at(0, 0), 'R');
}

TEST(tty, clear_zeroes_all_cells) {
    // Dirty the first row, then clear.
    for (size_t c = 0; c < VGA::WIDTH; ++c) {
        terminal_putchar('X');
    }
    terminal_clear();
    for (size_t r = 0; r < VGA::HEIGHT; ++r) {
        for (size_t c = 0; c < VGA::WIDTH; ++c) {
            ASSERT_EQ(vga_char_at(r, c), 0);
        }
    }
}

// ---------------------------------------------------------------------------
// ANSI escape sequences
// ---------------------------------------------------------------------------

TEST(tty, csi_cursor_position) {
    terminal_clear();
    terminal_write("\033[6;11H");
    terminal_putchar('P');
    ASSERT_EQ(vga_char_at(5, 10), 'P');
}

TEST(tty, csi_cursor_position_default) {
    terminal_clear();
    terminal_set_position(3, 3);
    terminal_write("\033[H");
    terminal_putchar('Q');
    ASSERT_EQ(vga_char_at(0, 0), 'Q');
}

TEST(tty, csi_cursor_position_partial) {
    terminal_clear();
    // Only row provided; col defaults to 1 (index 0).
    terminal_write("\033[3H");
    terminal_putchar('R');
    ASSERT_EQ(vga_char_at(2, 0), 'R');
}

TEST(tty, csi_clear_screen) {
    for (size_t c = 0; c < VGA::WIDTH; ++c) {
        terminal_putchar('X');
    }
    terminal_write("\033[2J");
    for (size_t r = 0; r < VGA::HEIGHT; ++r) {
        for (size_t c = 0; c < VGA::WIDTH; ++c) {
            ASSERT_EQ(vga_char_at(r, c), 0);
        }
    }
}

TEST(tty, csi_sgr_foreground) {
    terminal_clear();
    terminal_write("\033[32m");  // ANSI green -> VGA green (2)
    terminal_putchar('G');
    ASSERT_EQ(vga_char_at(0, 0), 'G');
    ASSERT_EQ(vga_color_at(0, 0),
              VGA::entry_color(VGA::Color::Green, VGA::Color::Black));
}

TEST(tty, csi_sgr_background) {
    terminal_clear();
    terminal_write("\033[41m");  // ANSI red background -> VGA red bg (4)
    terminal_putchar('B');
    ASSERT_EQ(vga_char_at(0, 0), 'B');
    ASSERT_EQ(vga_color_at(0, 0),
              VGA::entry_color(VGA::Color::LightGrey, VGA::Color::Red));
}

TEST(tty, csi_sgr_bright) {
    terminal_clear();
    terminal_write("\033[92m");  // ANSI bright green -> VGA light green (10)
    terminal_putchar('L');
    ASSERT_EQ(vga_char_at(0, 0), 'L');
    ASSERT_EQ(vga_color_at(0, 0),
              VGA::entry_color(VGA::Color::LightGreen, VGA::Color::Black));
}

TEST(tty, csi_sgr_reset) {
    terminal_clear();
    terminal_write("\033[32m");  // change color
    terminal_write("\033[0m");   // reset
    terminal_putchar('Z');
    ASSERT_EQ(vga_char_at(0, 0), 'Z');
    ASSERT_EQ(vga_color_at(0, 0),
              VGA::entry_color(VGA::Color::LightGrey, VGA::Color::Black));
}

TEST(tty, csi_cursor_up_down_left_right) {
    terminal_clear();

    // Cursor Up 3: from (12, 40) to (9, 40).
    terminal_set_position(12, 40);
    terminal_write("\033[3A");
    terminal_putchar('U');
    ASSERT_EQ(vga_char_at(9, 40), 'U');

    // Cursor Down 3: from (12, 40) to (15, 40).
    terminal_set_position(12, 40);
    terminal_write("\033[3B");
    terminal_putchar('D');
    ASSERT_EQ(vga_char_at(15, 40), 'D');

    // Cursor Forward 3: from (12, 40) to (12, 43).
    terminal_set_position(12, 40);
    terminal_write("\033[3C");
    terminal_putchar('R');
    ASSERT_EQ(vga_char_at(12, 43), 'R');

    // Cursor Back 3: from (12, 40) to (12, 37).
    terminal_set_position(12, 40);
    terminal_write("\033[3D");
    terminal_putchar('L');
    ASSERT_EQ(vga_char_at(12, 37), 'L');
}

TEST(tty, csi_unknown_sequence_ignored) {
    terminal_clear();
    terminal_putchar('X');
    // An unrecognized final byte is silently ignored; cursor stays after 'X'.
    terminal_write("\033[99z");
    terminal_putchar('Y');
    ASSERT_EQ(vga_char_at(0, 0), 'X');
    ASSERT_EQ(vga_char_at(0, 1), 'Y');
}

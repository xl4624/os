#include "ktest.h"
#include "terminal.h"
#include "tty.h"

// Read the FbTerminal cell buffer directly for verification.

static char fb_char_at(size_t row, size_t col) { return kTerminal.cell_char(row, col); }

static uint8_t fb_color_at(size_t row, size_t col) { return kTerminal.cell_color(row, col); }

static constexpr uint8_t make_color(uint8_t fg, uint8_t bg) {
  return static_cast<uint8_t>(fg | (bg << 4));
}

// Suppress debugcon output during tty tests to avoid cluttering the ktest log.
struct SuppressDebugcon {
  SuppressDebugcon() { tty_debugcon_enabled = false; }
  ~SuppressDebugcon() { tty_debugcon_enabled = true; }
};

// ===========================================================================
// terminal_putchar
// ===========================================================================

TEST(tty, putchar_appears_in_buffer) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_putchar('A');
  ASSERT_EQ(fb_char_at(0, 0), 'A');
}

TEST(tty, multiple_chars_fill_columns) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_putchar('H');
  terminal_putchar('i');
  ASSERT_EQ(fb_char_at(0, 0), 'H');
  ASSERT_EQ(fb_char_at(0, 1), 'i');
}

TEST(tty, newline_advances_row) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_putchar('\n');
  terminal_putchar('X');
  ASSERT_EQ(fb_char_at(1, 0), 'X');
}

TEST(tty, backspace_erases_last_char) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_putchar('A');
  terminal_putchar('\b');
  ASSERT_EQ(fb_char_at(0, 0), 0);
}

TEST(tty, tab_advances_to_next_tab_stop) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_putchar('\t');
  terminal_putchar('A');
  ASSERT_EQ(fb_char_at(0, 4), 'A');

  terminal_clear();
  terminal_putchar('X');
  terminal_putchar('\t');
  terminal_putchar('B');
  ASSERT_EQ(fb_char_at(0, 4), 'B');

  terminal_clear();
  terminal_set_position(0, 4);
  terminal_putchar('\t');
  terminal_putchar('C');
  ASSERT_EQ(fb_char_at(0, 8), 'C');
}

// ===========================================================================
// terminal_set_color
// ===========================================================================

TEST(tty, set_color_changes_attribute) {
  const SuppressDebugcon guard;
  terminal_clear();
  const uint8_t color = make_color(4, 1);  // Red on Blue
  terminal_set_color(color);
  terminal_putchar('Z');
  ASSERT_EQ(fb_char_at(0, 0), 'Z');
  ASSERT_EQ(fb_color_at(0, 0), color);
}

// ===========================================================================
// terminal_clear
// ===========================================================================

TEST(tty, clear_resets_to_origin) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_putchar('A');
  terminal_putchar('B');
  terminal_clear();
  terminal_putchar('Q');
  ASSERT_EQ(fb_char_at(0, 0), 'Q');
}

// ===========================================================================
// terminal_set_position
// ===========================================================================

TEST(tty, set_position_moves_cursor) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_set_position(3, 5);
  terminal_putchar('P');
  ASSERT_EQ(fb_char_at(3, 5), 'P');
}

TEST(tty, set_position_clamps_out_of_range) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_set_position(999, 0);
  terminal_putchar('C');
  ASSERT_EQ(fb_char_at(kTerminal.rows() - 1, 0), 'C');
}

TEST(tty, set_position_col_clamps_out_of_range) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_set_position(0, 999);
  terminal_putchar('D');
  ASSERT_EQ(fb_char_at(0, kTerminal.cols() - 1), 'D');
}

TEST(tty, line_wrap_at_right_edge) {
  const SuppressDebugcon guard;
  terminal_clear();
  for (size_t i = 0; i < kTerminal.cols(); ++i) {
    terminal_putchar('A');
  }
  terminal_putchar('B');
  ASSERT_EQ(fb_char_at(1, 0), 'B');
}

TEST(tty, scroll_on_overflow) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_putchar('S');
  for (size_t i = 0; i < kTerminal.rows(); ++i) {
    terminal_putchar('\n');
  }
  ASSERT_EQ(fb_char_at(0, 0), 0);
}

TEST(tty, color_persists_across_newline) {
  const SuppressDebugcon guard;
  terminal_clear();
  const uint8_t color = make_color(2, 0);  // Green on Black
  terminal_set_color(color);
  terminal_putchar('A');
  terminal_putchar('\n');
  terminal_putchar('B');
  ASSERT_EQ(fb_color_at(0, 0), color);
  ASSERT_EQ(fb_color_at(1, 0), color);
}

TEST(tty, backspace_at_col_zero_goes_to_previous_row) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_putchar('\n');
  terminal_putchar('\b');
  terminal_putchar('R');
  ASSERT_EQ(fb_char_at(0, 0), 'R');
}

TEST(tty, clear_zeroes_all_cells) {
  const SuppressDebugcon guard;
  for (size_t c = 0; c < kTerminal.cols(); ++c) {
    terminal_putchar('X');
  }
  terminal_clear();
  for (size_t r = 0; r < kTerminal.rows(); ++r) {
    for (size_t c = 0; c < kTerminal.cols(); ++c) {
      ASSERT_EQ(fb_char_at(r, c), 0);
    }
  }
}

// ===========================================================================
// ANSI escape sequences
// ===========================================================================

TEST(tty, csi_cursor_position) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_write("\033[6;11H");
  terminal_putchar('P');
  ASSERT_EQ(fb_char_at(5, 10), 'P');
}

TEST(tty, csi_cursor_position_default) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_set_position(3, 3);
  terminal_write("\033[H");
  terminal_putchar('Q');
  ASSERT_EQ(fb_char_at(0, 0), 'Q');
}

TEST(tty, csi_cursor_position_partial) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_write("\033[3H");
  terminal_putchar('R');
  ASSERT_EQ(fb_char_at(2, 0), 'R');
}

TEST(tty, csi_clear_screen) {
  const SuppressDebugcon guard;
  for (size_t c = 0; c < kTerminal.cols(); ++c) {
    terminal_putchar('X');
  }
  terminal_write("\033[2J");
  for (size_t r = 0; r < kTerminal.rows(); ++r) {
    for (size_t c = 0; c < kTerminal.cols(); ++c) {
      ASSERT_EQ(fb_char_at(r, c), 0);
    }
  }
}

TEST(tty, csi_sgr_foreground) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_write("\033[32m");  // ANSI green -> VGA green (2)
  terminal_putchar('G');
  ASSERT_EQ(fb_char_at(0, 0), 'G');
  ASSERT_EQ(fb_color_at(0, 0), make_color(2, 0));
}

TEST(tty, csi_sgr_background) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_write("\033[41m");  // ANSI red background -> VGA red bg (4)
  terminal_putchar('B');
  ASSERT_EQ(fb_char_at(0, 0), 'B');
  ASSERT_EQ(fb_color_at(0, 0), make_color(7, 4));
}

TEST(tty, csi_sgr_bright) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_write("\033[92m");  // ANSI bright green -> VGA light green (10)
  terminal_putchar('L');
  ASSERT_EQ(fb_char_at(0, 0), 'L');
  ASSERT_EQ(fb_color_at(0, 0), make_color(10, 0));
}

TEST(tty, csi_sgr_reset) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_write("\033[32m");
  terminal_write("\033[0m");
  terminal_putchar('Z');
  ASSERT_EQ(fb_char_at(0, 0), 'Z');
  ASSERT_EQ(fb_color_at(0, 0), make_color(7, 0));
}

TEST(tty, csi_cursor_up_down_left_right) {
  const SuppressDebugcon guard;
  terminal_clear();

  terminal_set_position(12, 40);
  terminal_write("\033[3A");
  terminal_putchar('U');
  ASSERT_EQ(fb_char_at(9, 40), 'U');

  terminal_set_position(12, 40);
  terminal_write("\033[3B");
  terminal_putchar('D');
  ASSERT_EQ(fb_char_at(15, 40), 'D');

  terminal_set_position(12, 40);
  terminal_write("\033[3C");
  terminal_putchar('R');
  ASSERT_EQ(fb_char_at(12, 43), 'R');

  terminal_set_position(12, 40);
  terminal_write("\033[3D");
  terminal_putchar('L');
  ASSERT_EQ(fb_char_at(12, 37), 'L');
}

TEST(tty, csi_unknown_sequence_ignored) {
  const SuppressDebugcon guard;
  terminal_clear();
  terminal_putchar('X');
  terminal_write("\033[99z");
  terminal_putchar('Y');
  ASSERT_EQ(fb_char_at(0, 0), 'X');
  ASSERT_EQ(fb_char_at(0, 1), 'Y');
}

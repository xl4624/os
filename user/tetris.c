#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termctl.h>
#include <unistd.h>

// ======================================================================
// Constants
// ======================================================================

#define BOARD_W 10
#define BOARD_H 20
#define CELL_CHARS 2

/* Board display position (top-left corner) */
#define BOARD_ROW 1
#define BOARD_COL 2

/* Info panel position (to the right of the board) */
#define INFO_COL (BOARD_COL + (BOARD_W * CELL_CHARS) + 4)

/* Timing */
#define FRAME_MS 20
#define INITIAL_DROP_MS 500
#define MIN_DROP_MS 50
#define LEVEL_SPEEDUP_MS 40
#define LINES_PER_LEVEL 10

/* Number of tetromino types */
#define NUM_PIECES 7

// ======================================================================
// Tetromino data
// ======================================================================

/*
 * Each piece is stored as a 16-bit bitmap representing a 4x4 grid.
 * Bit 15 = row 0 col 0, bit 14 = row 0 col 1, ...
 */

static const unsigned short pieces[NUM_PIECES][4] = {
    /* I */
    {0x4444, 0x0F00, 0x2222, 0x00F0},
    /* O */
    {0x6600, 0x6600, 0x6600, 0x6600},
    /* T */
    {0x04E0, 0x0464, 0x00E4, 0x04C4},
    /* S */
    {0x0360, 0x0231, 0x0360, 0x0231},
    /* Z */
    {0x0630, 0x0264, 0x0630, 0x0264},
    /* J */
    {0x44C0, 0x8E00, 0x6440, 0x0E20},
    /* L */
    {0x4460, 0x0E80, 0xC440, 0x2E00},
};

/* VGA color for each piece type (index 0-6) */
static const unsigned char piece_colors[NUM_PIECES] = {
    VGA_CYAN,        /* I */
    VGA_LIGHT_BROWN, /* O */
    VGA_MAGENTA,     /* T */
    VGA_GREEN,       /* S */
    VGA_RED,         /* Z */
    VGA_BLUE,        /* J */
    VGA_BROWN,       /* L */
};

/* Score table: 1 line=100, 2=300, 3=500, 4=800 */
static const int score_table[5] = {0, 100, 300, 500, 800};

static int board[BOARD_H][BOARD_W]; /* 0=empty, nonzero=VGA color */
static int cur_type, cur_rot, cur_row, cur_col;
static int next_type;
static int score, lines, level;
static int game_over, paused;

// ======================================================================
// Piece helpers
// ======================================================================

static int piece_cell(int type, int rot, int r, int c) {
  unsigned short mask = pieces[type][rot];
  return (mask >> (15 - (r * 4 + c))) & 1;
}

static int check_fit(int type, int rot, int row, int col) {
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (!piece_cell(type, rot, r, c)) {
        continue;
      }
      int br = row + r;
      int bc = col + c;
      if (bc < 0 || bc >= BOARD_W || br >= BOARD_H) {
        return 0;
      }
      if (br >= 0 && board[br][bc] != 0) {
        return 0;
      }
    }
  }
  return 1;
}

static void lock_piece(void) {
  unsigned char color = piece_colors[cur_type];
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (piece_cell(cur_type, cur_rot, r, c)) {
        int br = cur_row + r;
        int bc = cur_col + c;
        if (br >= 0 && br < BOARD_H && bc >= 0 && bc < BOARD_W) {
          board[br][bc] = color;
        }
      }
    }
  }
}

static int clear_full_lines(void) {
  int cleared = 0;
  for (int r = BOARD_H - 1; r >= 0; r--) {
    int full = 1;
    for (int c = 0; c < BOARD_W; c++) {
      if (board[r][c] == 0) {
        full = 0;
        break;
      }
    }
    if (full) {
      cleared++;
      for (int y = r; y > 0; y--) {
        for (int c = 0; c < BOARD_W; c++) {
          board[y][c] = board[y - 1][c];
        }
      }
      for (int c = 0; c < BOARD_W; c++) {
        board[0][c] = 0;
      }
      r++; /* re-check this row since we shifted down */
    }
  }
  return cleared;
}

static void spawn_piece(void) {
  cur_type = next_type;
  next_type = rand() % NUM_PIECES;
  cur_rot = 0;
  cur_row = -1;
  cur_col = BOARD_W / 2 - 2;

  if (!check_fit(cur_type, cur_rot, cur_row, cur_col)) {
    game_over = 1;
  }
}

/* Lock current piece and handle line clearing / scoring / next spawn. */
static void lock_and_spawn(void) {
  lock_piece();
  int cleared = clear_full_lines();
  if (cleared > 0) {
    lines += cleared;
    score += score_table[cleared] * (level + 1);
    level = lines / LINES_PER_LEVEL;
  }
  spawn_piece();
}

// ======================================================================
// Rendering
// ======================================================================

static void draw_cell(int row, int col, int color) {
  set_cursor(BOARD_ROW + row, BOARD_COL + col * CELL_CHARS);
  if (color) {
    set_color(VGA_COLOR(color, color));
    write(1, "  ", CELL_CHARS);
  } else {
    set_color(VGA_COLOR(VGA_DARK_GREY, VGA_BLACK));
    write(1, " .", CELL_CHARS);
  }
}

static void draw_board(void) {
  for (int r = 0; r < BOARD_H; r++) {
    for (int c = 0; c < BOARD_W; c++) {
      draw_cell(r, c, board[r][c]);
    }
  }
}

static void draw_piece(int show) {
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (piece_cell(cur_type, cur_rot, r, c)) {
        int br = cur_row + r;
        int bc = cur_col + c;
        if (br >= 0 && br < BOARD_H && bc >= 0 && bc < BOARD_W) {
          draw_cell(br, bc, show ? piece_colors[cur_type] : board[br][bc]);
        }
      }
    }
  }
}

static void draw_ghost(int show) {
  int ghost_row = cur_row;
  while (check_fit(cur_type, cur_rot, ghost_row + 1, cur_col)) {
    ghost_row++;
  }
  if (ghost_row == cur_row) {
    return;
  }
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (!piece_cell(cur_type, cur_rot, r, c)) {
        continue;
      }
      int br = ghost_row + r;
      int bc = cur_col + c;
      if (br < 0 || br >= BOARD_H || bc < 0 || bc >= BOARD_W) {
        continue;
      }
      /* Skip cells where the actual piece already is */
      int pr = cur_row + r;
      if (pr == br) {
        continue;
      }
      if (show) {
        set_cursor(BOARD_ROW + br, BOARD_COL + bc * CELL_CHARS);
        set_color(VGA_COLOR(VGA_DARK_GREY, VGA_BLACK));
        write(1, "[]", CELL_CHARS);
      } else {
        draw_cell(br, bc, board[br][bc]);
      }
    }
  }
}

/* Erase the active piece and ghost before moving, redraw after. */
static void erase_active(void) {
  draw_ghost(0);
  draw_piece(0);
}

static void show_active(void) {
  draw_ghost(1);
  draw_piece(1);
}

static void draw_border(void) {
  set_color(VGA_COLOR(VGA_WHITE, VGA_BLACK));

  /* Top border */
  set_cursor(BOARD_ROW - 1, BOARD_COL - 1);
  write(1, "+", 1);
  for (int c = 0; c < BOARD_W * CELL_CHARS; c++) {
    write(1, "-", 1);
  }
  write(1, "+", 1);

  /* Side borders */
  for (int r = 0; r < BOARD_H; r++) {
    set_cursor(BOARD_ROW + r, BOARD_COL - 1);
    write(1, "|", 1);
    set_cursor(BOARD_ROW + r, BOARD_COL + BOARD_W * CELL_CHARS);
    write(1, "|", 1);
  }

  /* Bottom border */
  set_cursor(BOARD_ROW + BOARD_H, BOARD_COL - 1);
  write(1, "+", 1);
  for (int c = 0; c < BOARD_W * CELL_CHARS; c++) {
    write(1, "-", 1);
  }
  write(1, "+", 1);
}

static void draw_info(void) {
  set_color(VGA_COLOR(VGA_WHITE, VGA_BLACK));

  set_cursor(BOARD_ROW, INFO_COL);
  printf("SCORE: %d    ", score);
  set_cursor(BOARD_ROW + 1, INFO_COL);
  printf("LINES: %d    ", lines);
  set_cursor(BOARD_ROW + 2, INFO_COL);
  printf("LEVEL: %d    ", level);

  set_cursor(BOARD_ROW + 4, INFO_COL);
  printf("NEXT:");

  /* Clear next piece preview area */
  for (int r = 0; r < 4; r++) {
    set_cursor(BOARD_ROW + 5 + r, INFO_COL);
    set_color(VGA_COLOR(VGA_BLACK, VGA_BLACK));
    write(1, "        ", 8);
  }

  /* Draw next piece */
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (piece_cell(next_type, 0, r, c)) {
        set_cursor(BOARD_ROW + 5 + r, INFO_COL + c * CELL_CHARS);
        set_color(VGA_COLOR(piece_colors[next_type], piece_colors[next_type]));
        write(1, "  ", CELL_CHARS);
      }
    }
  }

  set_color(VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK));
  set_cursor(BOARD_ROW + 11, INFO_COL);
  printf("CONTROLS:");
  set_cursor(BOARD_ROW + 12, INFO_COL);
  printf("<- -> Move");
  set_cursor(BOARD_ROW + 13, INFO_COL);
  printf("Up    Rotate");
  set_cursor(BOARD_ROW + 14, INFO_COL);
  printf("Down  Soft drop");
  set_cursor(BOARD_ROW + 15, INFO_COL);
  printf("Space Hard drop");
  set_cursor(BOARD_ROW + 16, INFO_COL);
  printf("P     Pause");
  set_cursor(BOARD_ROW + 17, INFO_COL);
  printf("R     Restart");
  set_cursor(BOARD_ROW + 18, INFO_COL);
  printf("Q     Quit");
}

static void draw_full_screen(void) {
  clear_screen();
  draw_border();
  draw_board();
  show_active();
  draw_info();
}

// ======================================================================
// Input handling
// ======================================================================

#define INPUT_NONE 0
#define INPUT_LEFT 1
#define INPUT_RIGHT 2
#define INPUT_UP 3
#define INPUT_DOWN 4
#define INPUT_SPACE 5
#define INPUT_P 6
#define INPUT_R 7
#define INPUT_Q 8

static int read_input(void) {
  char buffer[8];
  int n = read(0, buffer, sizeof(buffer));
  if (n <= 0) {
    return INPUT_NONE;
  }

  for (int i = 0; i < n; i++) {
    if (buffer[i] == '\x1b' && i + 2 < n && buffer[i + 1] == '[') {
      switch (buffer[i + 2]) {
        case 'A':
          return INPUT_UP;
        case 'B':
          return INPUT_DOWN;
        case 'C':
          return INPUT_RIGHT;
        case 'D':
          return INPUT_LEFT;
      }
      i += 2;
    } else if (buffer[i] == ' ') {
      return INPUT_SPACE;
    } else if (buffer[i] == 'p' || buffer[i] == 'P') {
      return INPUT_P;
    } else if (buffer[i] == 'r' || buffer[i] == 'R') {
      return INPUT_R;
    } else if (buffer[i] == 'q' || buffer[i] == 'Q') {
      return INPUT_Q;
    }
  }

  return INPUT_NONE;
}

// ======================================================================
// Game logic
// ======================================================================

static int get_drop_interval(void) { return INITIAL_DROP_MS; }

static void init_game(void) {
  memset(board, 0, sizeof(board));
  score = 0;
  lines = 0;
  level = 0;
  game_over = 0;
  paused = 0;
  next_type = rand() % NUM_PIECES;
  spawn_piece();
  draw_full_screen();
}

static void show_game_over(void) {
  set_color(VGA_COLOR(VGA_LIGHT_RED, VGA_BLACK));
  set_cursor(BOARD_ROW + BOARD_H / 2 - 1, BOARD_COL + 2);
  printf("  GAME OVER!  ");
  set_color(VGA_COLOR(VGA_WHITE, VGA_BLACK));
  set_cursor(BOARD_ROW + BOARD_H / 2 + 1, BOARD_COL + 2);
  printf("  Score: %d  ", score);
  set_cursor(BOARD_ROW + BOARD_H / 2 + 2, BOARD_COL + 2);
  printf(" R=Restart Q=Quit");
}

/* Returns: 0 = quit, 1 = restart */
static int run_game(void) {
  int drop_timer = 0;

  while (!game_over) {
    int input = read_input();

    if (input == INPUT_Q) {
      return 0;
    }
    if (input == INPUT_R) {
      return 1;
    }

    if (input == INPUT_P) {
      paused = !paused;
      if (paused) {
        set_color(VGA_COLOR(VGA_WHITE, VGA_BLACK));
        set_cursor(BOARD_ROW + BOARD_H / 2, BOARD_COL + 4);
        printf("  PAUSED  ");
      } else {
        draw_full_screen();
      }
    }

    if (paused) {
      msleep(FRAME_MS);
      continue;
    }

    switch (input) {
      case INPUT_LEFT:
        if (check_fit(cur_type, cur_rot, cur_row, cur_col - 1)) {
          erase_active();
          cur_col--;
          show_active();
        }
        break;
      case INPUT_RIGHT:
        if (check_fit(cur_type, cur_rot, cur_row, cur_col + 1)) {
          erase_active();
          cur_col++;
          show_active();
        }
        break;
      case INPUT_UP: {
        int new_rot = (cur_rot + 1) % 4;
        if (check_fit(cur_type, new_rot, cur_row, cur_col)) {
          erase_active();
          cur_rot = new_rot;
          show_active();
        }
        break;
      }
      case INPUT_DOWN:
        if (check_fit(cur_type, cur_rot, cur_row + 1, cur_col)) {
          erase_active();
          cur_row++;
          show_active();
          drop_timer = 0;
          score += 1;
          draw_info();
        }
        break;
      case INPUT_SPACE: {
        erase_active();
        int drop_count = 0;
        while (check_fit(cur_type, cur_rot, cur_row + 1, cur_col)) {
          cur_row++;
          drop_count++;
        }
        score += drop_count * 2;
        lock_and_spawn();
        draw_board();
        if (!game_over) {
          show_active();
        }
        draw_info();
        drop_timer = 0;
        break;
      }
    }

    /* Auto drop */
    drop_timer += FRAME_MS;
    if (drop_timer >= get_drop_interval()) {
      drop_timer = 0;
      if (check_fit(cur_type, cur_rot, cur_row + 1, cur_col)) {
        erase_active();
        cur_row++;
        show_active();
      } else {
        draw_piece(0);
        lock_and_spawn();
        draw_board();
        if (!game_over) {
          show_active();
        }
        draw_info();
      }
    }

    /* Hide cursor in a corner */
    set_cursor(VGA_HEIGHT - 1, VGA_WIDTH - 1);
    msleep(FRAME_MS);
  }

  /* Game over */
  show_game_over();

  /* Wait for R or Q */
  for (;;) {
    int input = read_input();
    if (input == INPUT_R) {
      return 1;
    }
    if (input == INPUT_Q) {
      return 0;
    }
    msleep(FRAME_MS);
  }
}

// ======================================================================
// Main
// ======================================================================

int main(void) {
  /* Seed RNG: wait for first keypress, use loop counter as seed */
  clear_screen();
  set_color(VGA_COLOR(VGA_WHITE, VGA_BLACK));
  set_cursor(10, 25);
  printf("=== TETRIS ===");
  set_cursor(12, 20);
  printf("Press any key to start...");

  unsigned int seed_counter = 0;
  for (;;) {
    char c;
    int n = read(0, &c, 1);
    if (n > 0) {
      break;
    }
    seed_counter++;
    msleep(1);
  }
  srand(seed_counter);

  for (;;) {
    init_game();
    if (!run_game()) {
      break;
    }
  }

  clear_screen();
  set_color(VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK));
  set_cursor(0, 0);
  printf("Thanks for playing!\n");
  return 0;
}

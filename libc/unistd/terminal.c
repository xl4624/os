#include <stddef.h>

#ifdef __is_libc

#include <stdint.h>
#include <unistd.h>

// Write an unsigned integer as decimal digits into buf, returning the new
// end pointer. Does not null-terminate; always writes at least one digit.
static char* write_uint(char* p, uint32_t n) {
  if (n >= 100U) {
    *p++ = (char)('0' + (int)(n / 100U));
  }
  if (n >= 10U) {
    *p++ = (char)('0' + (int)((n / 10U) % 10U));
  }
  *p++ = (char)('0' + (int)(n % 10U));
  return p;
}

static void set_cursor(unsigned int row, unsigned int col) {
  char buf[16];
  char* p = buf;
  *p++ = '\033';
  *p++ = '[';
  p = write_uint(p, (uint32_t)(row + 1U));
  *p++ = ';';
  p = write_uint(p, (uint32_t)(col + 1U));
  *p++ = 'H';
  (void)write(1, buf, (size_t)(p - buf));
}

static void set_color(unsigned char color) {
  char buf[16];
  char* p = buf;
  uint8_t fg = (uint8_t)(color & 0x0FU);
  uint8_t bg = (uint8_t)((color >> 4) & 0x0FU);
  *p++ = '\033';
  *p++ = '[';
  /* fg: 0-7 -> 30-37, 8-15 -> 90-97 */
  p = write_uint(p, fg < 8U ? 30U + fg : 82U + fg);
  *p++ = ';';
  /* bg: 0-7 -> 40-47, 8-15 -> 100-107 */
  p = write_uint(p, bg < 8U ? 40U + bg : 92U + bg);
  *p++ = 'm';
  (void)write(1, buf, (size_t)(p - buf));
}

static void show_cursor(void) { (void)write(1, "\033[?25h", 6); }

static void hide_cursor(void) { (void)write(1, "\033[?25l", 6); }

static void clear_screen(void) { (void)write(1, "\033[2J\033[H", 7); }

#endif

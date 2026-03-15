#include <termctl.h>

#if defined(__is_libc)

#include <stdint.h>
#include <unistd.h>

// Write an unsigned integer as decimal digits into buf, returning the new
// end pointer. Does not null-terminate; always writes at least one digit.
static char* write_uint(char* p, uint32_t n) {
  if (n >= 100u) *p++ = (char)('0' + (int)(n / 100u));
  if (n >= 10u) *p++ = (char)('0' + (int)((n / 10u) % 10u));
  *p++ = (char)('0' + (int)(n % 10u));
  return p;
}

void set_cursor(unsigned int row, unsigned int col) {
  char buf[16];
  char* p = buf;
  *p++ = '\033';
  *p++ = '[';
  p = write_uint(p, (uint32_t)(row + 1u));
  *p++ = ';';
  p = write_uint(p, (uint32_t)(col + 1u));
  *p++ = 'H';
  (void)write(1, buf, (size_t)(p - buf));
}

void set_color(unsigned char color) {
  char buf[16];
  char* p = buf;
  uint8_t fg = (uint8_t)(color & 0x0Fu);
  uint8_t bg = (uint8_t)((color >> 4) & 0x0Fu);
  *p++ = '\033';
  *p++ = '[';
  /* fg: 0-7 -> 30-37, 8-15 -> 90-97 */
  p = write_uint(p, fg < 8u ? 30u + fg : 82u + fg);
  *p++ = ';';
  /* bg: 0-7 -> 40-47, 8-15 -> 100-107 */
  p = write_uint(p, bg < 8u ? 40u + bg : 92u + bg);
  *p++ = 'm';
  (void)write(1, buf, (size_t)(p - buf));
}

void show_cursor(void) { (void)write(1, "\033[?25h", 6); }

void hide_cursor(void) { (void)write(1, "\033[?25l", 6); }

void clear_screen(void) { (void)write(1, "\033[2J\033[H", 7); }

#endif

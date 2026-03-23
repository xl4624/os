#include <fcntl.h>
#include <stdio.h>
#include <sys/kbd.h>
#include <sys/types.h>
#include <unistd.h>

static const char* key_name(uint8_t k) {
  if (k >= KEY_A && k <= KEY_Z) {
    static char buf[2] = {0, 0};
    buf[0] = (char)('A' + k - KEY_A);
    return buf;
  }
  if (k >= KEY_0 && k <= KEY_9) {
    static char buf[2] = {0, 0};
    buf[0] = (char)('0' + k - KEY_0);
    return buf;
  }
  if (k >= KEY_F1 && k <= KEY_F12) {
    static char buf[4];
    snprintf(buf, sizeof(buf), "F%d", k - KEY_F1 + 1);
    return buf;
  }
  switch (k) {
    case KEY_UNKNOWN:
      return "Unknown";
    case KEY_SPACE:
      return "Space";
    case KEY_ENTER:
      return "Enter";
    case KEY_BACKSPACE:
      return "Backspace";
    case KEY_TAB:
      return "Tab";
    case KEY_ESC:
      return "Esc";
    case KEY_MINUS:
      return "Minus";
    case KEY_EQUALS:
      return "Equals";
    case KEY_LBRACKET:
      return "LBracket";
    case KEY_RBRACKET:
      return "RBracket";
    case KEY_BACKSLASH:
      return "Backslash";
    case KEY_SEMICOLON:
      return "Semicolon";
    case KEY_APOSTROPHE:
      return "Apostrophe";
    case KEY_GRAVE:
      return "Grave";
    case KEY_COMMA:
      return "Comma";
    case KEY_PERIOD:
      return "Period";
    case KEY_SLASH:
      return "Slash";
    case KEY_LSHIFT:
      return "LShift";
    case KEY_RSHIFT:
      return "RShift";
    case KEY_LCTRL:
      return "LCtrl";
    case KEY_RCTRL:
      return "RCtrl";
    case KEY_LALT:
      return "LAlt";
    case KEY_RALT:
      return "RAlt";
    case KEY_CAPSLOCK:
      return "CapsLock";
    case KEY_INSERT:
      return "Insert";
    case KEY_DELETE:
      return "Delete";
    case KEY_HOME:
      return "Home";
    case KEY_END:
      return "End";
    case KEY_PAGEUP:
      return "PageUp";
    case KEY_PAGEDOWN:
      return "PageDown";
    case KEY_UP:
      return "Up";
    case KEY_DOWN:
      return "Down";
    case KEY_LEFT:
      return "Left";
    case KEY_RIGHT:
      return "Right";
    default:
      return "?";
  }
}

int main(void) {
  printf("kbd_test: reading /dev/kbd -- press Esc to quit\n");
  fflush(stdout);

  int fd = open("/dev/kbd", O_RDONLY);
  if (fd < 0) {
    printf("kbd_test: failed to open /dev/kbd\n");
    return 1;
  }

  for (;;) {
    struct kbd_event ev;
    ssize_t n = read(fd, &ev, sizeof(ev));
    if (n <= 0) {
      msleep(10);
      continue;
    }
    printf("[%s] key=%d (%s)\n", ev.pressed ? "press  " : "release", ev.key, key_name(ev.key));
    fflush(stdout);
    if (ev.key == KEY_ESC && ev.pressed) {
      break;
    }
  }

  close(fd);
  printf("kbd_test: done\n");
  return 0;
}

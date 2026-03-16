#include <fcntl.h>
#include <stdint.h>
#include <sys/fb.h>
#include <sys/kbd.h>
#include <unistd.h>

#include "doomgeneric/doomgeneric/doomgeneric.h"
#include "doomgeneric/doomgeneric/doomkeys.h"

static int s_kbd_fd = -1;

/*
 * Translate Key::Value (from keyboard.h) to a DOOM key code.
 * Key::Value enum: Unknown=0, A=1..Z=26, Num0=27..Num9=36,
 * Space=37, Enter=38, Backspace=39, Tab=40, Esc=41,
 * Minus=42, Equals=43, LeftBracket=44, RightBracket=45, Backslash=46,
 * Semicolon=47, Apostrophe=48, Grave=49, Comma=50, Period=51, Slash=52,
 * LeftShift=53, RightShift=54, LeftCtrl=55, RightCtrl=56,
 * LeftAlt=57, RightAlt=58, CapsLock=59,
 * F1=60..F12=71,
 * Insert=72, Delete=73, Home=74, End=75, PageUp=76, PageDown=77,
 * Up=78, Down=79, Left=80, Right=81
 */
static unsigned char translate_key(uint8_t k) {
  /* Letters A-Z -> 'a'-'z' */
  if (k >= 1 && k <= 26) return (unsigned char)('a' + k - 1);

  /* Digits Num0-Num9 -> '0'-'9' */
  if (k >= 27 && k <= 36) return (unsigned char)('0' + k - 27);

  /* F1-F10 are sequential; F11/F12 are not */
  if (k >= 60 && k <= 69) return (unsigned char)(KEY_F1 + (k - 60));

  switch (k) {
    case 37:
      return KEY_USE; /* Space -> open doors/use */
    case 38:
      return KEY_ENTER;
    case 39:
      return KEY_BACKSPACE;
    case 40:
      return KEY_TAB;
    case 41:
      return KEY_ESCAPE;
    case 42:
      return KEY_MINUS;
    case 43:
      return KEY_EQUALS;
    case 50:
      return ',';
    case 51:
      return '.';
    case 53: /* LeftShift  */
      return KEY_RSHIFT;
    case 54: /* RightShift */
      return KEY_RSHIFT;
    case 55: /* LeftCtrl   */
      return KEY_FIRE;
    case 56: /* RightCtrl  */
      return KEY_FIRE;
    case 57: /* LeftAlt    */
      return KEY_LALT;
    case 58: /* RightAlt   */
      return KEY_LALT;
    case 70:
      return KEY_F11;
    case 71:
      return KEY_F12;
    case 78:
      return KEY_UPARROW;
    case 79:
      return KEY_DOWNARROW;
    case 80:
      return KEY_LEFTARROW;
    case 81:
      return KEY_RIGHTARROW;
    default:
      return 0;
  }
}

void DG_Init(void) { s_kbd_fd = open("/dev/kbd", O_RDONLY); }

void DG_DrawFrame(void) { fb_flip(DG_ScreenBuffer, DOOMGENERIC_RESX, DOOMGENERIC_RESY); }

void DG_SleepMs(uint32_t ms) { msleep(ms); }

uint32_t DG_GetTicksMs(void) { return (uint32_t)(getticks() * 10); }

int DG_GetKey(int* pressed, unsigned char* doomKey) {
  if (s_kbd_fd < 0) return 0;
  struct kbd_event ev;
  if (read(s_kbd_fd, &ev, sizeof(ev)) <= 0) return 0;
  unsigned char dk = translate_key(ev.key);
  if (dk == 0) return 0;
  *pressed = ev.pressed;
  *doomKey = dk;
  return 1;
}

void DG_SetWindowTitle(const char* title) { (void)title; }

int main(int argc, char* argv[]) {
  /* If no -iwad argument was given, default to /doom1.wad. */
  int has_iwad = 0;
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && argv[i][1] == 'i') {
      has_iwad = 1;
      break;
    }
  }
  if (!has_iwad) {
    static char* default_argv[] = {"doom", "-iwad", "/doom1.wad", 0};
    argc = 3;
    argv = default_argv;
  }
  doomgeneric_Create(argc, argv);
  for (;;) doomgeneric_Tick();
}

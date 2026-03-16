#ifndef _SYS_KBD_H
#define _SYS_KBD_H

#include <stdint.h>

/*
 * Keyboard event returned by reading /dev/kbd.
 * Non-blocking: read returns 0 when no events are pending.
 */
struct kbd_event {
  uint8_t key;     /* KEY_* code below */
  uint8_t pressed; /* 1 = press, 0 = release */
};

/* Key codes -- match Key::Value in kernel/include/keyboard.h */
#define KEY_UNKNOWN 0
#define KEY_A 1
#define KEY_B 2
#define KEY_C 3
#define KEY_D 4
#define KEY_E 5
#define KEY_F 6
#define KEY_G 7
#define KEY_H 8
#define KEY_I 9
#define KEY_J 10
#define KEY_K 11
#define KEY_L 12
#define KEY_M 13
#define KEY_N 14
#define KEY_O 15
#define KEY_P 16
#define KEY_Q 17
#define KEY_R 18
#define KEY_S 19
#define KEY_T 20
#define KEY_U 21
#define KEY_V 22
#define KEY_W 23
#define KEY_X 24
#define KEY_Y 25
#define KEY_Z 26
#define KEY_0 27
#define KEY_1 28
#define KEY_2 29
#define KEY_3 30
#define KEY_4 31
#define KEY_5 32
#define KEY_6 33
#define KEY_7 34
#define KEY_8 35
#define KEY_9 36
#define KEY_SPACE 37
#define KEY_ENTER 38
#define KEY_BACKSPACE 39
#define KEY_TAB 40
#define KEY_ESC 41
#define KEY_MINUS 42
#define KEY_EQUALS 43
#define KEY_LBRACKET 44
#define KEY_RBRACKET 45
#define KEY_BACKSLASH 46
#define KEY_SEMICOLON 47
#define KEY_APOSTROPHE 48
#define KEY_GRAVE 49
#define KEY_COMMA 50
#define KEY_PERIOD 51
#define KEY_SLASH 52
#define KEY_LSHIFT 53
#define KEY_RSHIFT 54
#define KEY_LCTRL 55
#define KEY_RCTRL 56
#define KEY_LALT 57
#define KEY_RALT 58
#define KEY_CAPSLOCK 59
#define KEY_F1 60
#define KEY_F2 61
#define KEY_F3 62
#define KEY_F4 63
#define KEY_F5 64
#define KEY_F6 65
#define KEY_F7 66
#define KEY_F8 67
#define KEY_F9 68
#define KEY_F10 69
#define KEY_F11 70
#define KEY_F12 71
#define KEY_INSERT 72
#define KEY_DELETE 73
#define KEY_HOME 74
#define KEY_END 75
#define KEY_PAGEUP 76
#define KEY_PAGEDOWN 77
#define KEY_UP 78
#define KEY_DOWN 79
#define KEY_LEFT 80
#define KEY_RIGHT 81

#endif

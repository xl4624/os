#ifndef _SYS_KBD_H
#define _SYS_KBD_H

#include <stdint.h>

/*
 * Keyboard event returned by reading /dev/kbd.
 * Non-blocking: read returns 0 when no events are pending.
 *
 * The key field uses Key::Value from keyboard.h and can be compared directly in userspace C code
 * using the values from that enum (0 = Unknown, 1 = A, 2 = B, ... see keyboard.h for the full
 * list).
 */
struct kbd_event {
  uint8_t key;     /* Key::Value code */
  uint8_t pressed; /* 1 = press, 0 = release */
};

#endif

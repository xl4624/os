#include <stdio.h>

#ifdef __is_libk

#include "keyboard.h"

int getchar(void) { return keyboard_getchar(); }

#else /* __is_libc */

#include <unistd.h>

int getchar(void) {
  char c = 0;
  int n = read(0, &c, 1);
  if (n <= 0) {
    return EOF;
  }
  return (unsigned char)c;
}

#endif

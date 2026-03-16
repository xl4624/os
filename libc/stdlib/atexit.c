#include <stdlib.h>

#define ATEXIT_MAX 8

static void (*s_atexit_funcs[ATEXIT_MAX])(void);
static int s_atexit_count = 0;

int atexit(void (*func)(void)) {
  if (s_atexit_count >= ATEXIT_MAX) {
    return -1;
  }
  s_atexit_funcs[s_atexit_count++] = func;
  return 0;
}

void __run_atexit_handlers(void) {  // NOLINT(bugprone-reserved-identifier)
  for (int i = s_atexit_count - 1; i >= 0; i--) {
    s_atexit_funcs[i]();
  }
}

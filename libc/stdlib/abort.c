#include <stdlib.h>

#ifdef __is_libk
#include "interrupt.h"
#include "panic.h"
#elif defined(__is_libc)
#include <signal.h>
#include <unistd.h>
#endif

__attribute__((noreturn)) void abort(void) {
#ifdef __is_libk
  panic("kernel: abort()\n");
#elif defined(__is_libc)
  kill(getpid(), SIGABRT);
  _exit(134);  // fallback if SIGABRT is ignored
#endif
  __builtin_unreachable();
}

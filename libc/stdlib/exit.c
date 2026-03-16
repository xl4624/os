#include <stdlib.h>
#include <unistd.h>

extern void __run_atexit_handlers(void);  // NOLINT(bugprone-reserved-identifier)

void exit(int status) {
  __run_atexit_handlers();
  _exit(status);
}

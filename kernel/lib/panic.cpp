#include "panic.h"

#include <stdarg.h>
#include <stdio.h>

#include "interrupt.h"

#ifdef KERNEL_TESTS
#include <sys/io.h>
static constexpr uint16_t kQemuExitPort = 0xF4;
#endif

__attribute__((noreturn)) void panic(const char* format, ...) {
  va_list args;
  va_start(args, format);
  printf("panic: ");
  vprintf(format, args);
  va_end(args);

#ifdef KERNEL_TESTS
  // A panic during a kernel test is a hard crash -- exit QEMU immediately with
  // the failure code so the test run does not hang waiting for a hlt loop.
  outb(kQemuExitPort, 0x01);  // isa-debug-exit: (0x01 << 1) | 1 = exit code 3
  __builtin_unreachable();
#else
  halt_and_catch_fire();
#endif
}

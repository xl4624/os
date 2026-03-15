#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

int open(const char* path, int flags, ...) {
  (void)flags;
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_OPEN), "b"(path));
  return ret;
}

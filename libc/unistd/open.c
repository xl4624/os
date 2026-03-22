#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

int open(const char* path, int flags, ...) {
  int mode = 0;
  if ((flags & O_CREAT) != 0) {
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);
  }
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_OPEN), "b"(path), "c"(flags), "d"(mode));
  return __syscall_ret(ret);
}

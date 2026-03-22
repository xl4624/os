#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/syscall.h>

int fcntl(int fd, int cmd, ...) {
  int arg = 0;
  va_list ap;
  va_start(ap, cmd);
  arg = va_arg(ap, int);
  va_end(ap);

  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_FCNTL), "b"(fd), "c"(cmd), "d"(arg));
  return __syscall_ret(ret);
}

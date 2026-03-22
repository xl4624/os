#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/syscall.h>

int fcntl(int fd, int cmd, ...) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_FCNTL), "b"(fd), "c"(cmd));
  return __syscall_ret(ret);
}

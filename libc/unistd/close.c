#include <unistd.h>

#ifdef __is_libk

int close(int fd) {
  (void)fd;
  return -1;
}

#else /* __is_libc */

#include <stdint.h>
#include <sys/syscall.h>

int close(int fd) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_CLOSE), "b"(fd));
  return __syscall_ret(ret);
}

#endif

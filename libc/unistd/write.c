#include <stddef.h>
#include <unistd.h>

#ifdef __is_libk

int write(int fd, const void* buf, size_t count) {
  (void)fd;
  (void)buf;
  (void)count;
  return -1;
}

#else /* __is_libc */

#include <stdint.h>
#include <sys/syscall.h>

int write(int fd, const void* buf, size_t count) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_WRITE), "b"(fd), "c"(buf), "d"(count));
  return __syscall_ret(ret);
}

#endif

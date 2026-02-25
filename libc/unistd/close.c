#include <unistd.h>

#if defined(__is_libk)

int close(int fd) {
  (void)fd;
  return -1;
}

#elif defined(__is_libc)

#include <sys/syscall.h>

int close(int fd) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_CLOSE), "b"(fd));
  return ret;
}

#endif

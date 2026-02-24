#include <unistd.h>

#if defined(__is_libk)

int read(int fd, void* buf, size_t count) {
  (void)fd;
  (void)buf;
  (void)count;
  return -1;
}

#elif defined(__is_libc)

#include <sys/syscall.h>

int read(int fd, void* buf, size_t count) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_READ), "b"(fd), "c"(buf), "d"(count));
  return ret;
}

#endif

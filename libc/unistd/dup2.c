#include <unistd.h>

#if defined(__is_libk)

int dup2(int oldfd, int newfd) {
  (void)oldfd;
  (void)newfd;
  return -1;
}

#elif defined(__is_libc)

#include <sys/syscall.h>

int dup2(int oldfd, int newfd) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_DUP2), "b"(oldfd), "c"(newfd));
  return ret;
}

#endif

#include <unistd.h>

#ifdef __is_libk

int dup2(int oldfd, int newfd) {
  (void)oldfd;
  (void)newfd;
  return -1;
}

#else /* __is_libc */

#include <stdint.h>
#include <sys/syscall.h>

int dup2(int oldfd, int newfd) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_DUP2), "b"(oldfd), "c"(newfd));
  return __syscall_ret(ret);
}

#endif

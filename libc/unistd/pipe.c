#include <unistd.h>

#ifdef __is_libk

int pipe(int pipefd[2]) {
  (void)pipefd;
  return -1;
}

#else /* __is_libc */

#include <stdint.h>
#include <sys/syscall.h>

int pipe(int pipefd[2]) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_PIPE), "b"(pipefd));
  return __syscall_ret(ret);
}

#endif

#include <unistd.h>

#if defined(__is_libk)

int pipe(int pipefd[2]) {
  (void)pipefd;
  return -1;
}

#elif defined(__is_libc)

#include <sys/syscall.h>

int pipe(int pipefd[2]) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_PIPE), "b"(pipefd));
  return ret;
}

#endif

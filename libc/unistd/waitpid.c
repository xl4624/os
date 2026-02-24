#include <unistd.h>

#if defined(__is_libk)

int waitpid(int pid, int* exit_code) {
  (void)pid;
  (void)exit_code;
  return -1;
}

#elif defined(__is_libc)

#include <sys/syscall.h>

int waitpid(int pid, int* exit_code) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_WAITPID), "b"(pid), "c"(exit_code));
  return ret;
}

#endif

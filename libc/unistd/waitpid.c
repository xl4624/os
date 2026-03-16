#include <unistd.h>

#ifdef __is_libk

int waitpid(int pid, int* exit_code) {
  (void)pid;
  (void)exit_code;
  return -1;
}

#elif defined(__is_libc)

#include <stdint.h>
#include <sys/syscall.h>

int waitpid(int pid, int* exit_code) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_WAITPID), "b"(pid), "c"(exit_code));
  return __syscall_ret(ret);
}

#endif

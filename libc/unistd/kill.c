#include <signal.h>

#ifdef __is_libk

int kill(int pid, int sig) {
  (void)pid;
  (void)sig;
  return -1;
}

#else /* __is_libc */

#include <stdint.h>
#include <sys/syscall.h>

int kill(int pid, int sig) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_KILL), "b"(pid), "c"(sig));
  return __syscall_ret(ret);
}

#endif

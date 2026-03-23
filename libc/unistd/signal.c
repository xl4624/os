#include <signal.h>

#ifdef __is_libk

sighandler_t signal(int signum, sighandler_t handler) {
  (void)signum;
  (void)handler;
  return SIG_ERR;
}

int raise(int sig) {
  (void)sig;
  return -1;
}

#else /* __is_libc */

#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

sighandler_t signal(int signum, sighandler_t handler) {
  uint32_t old = 0;
  int32_t ret;
  __asm__ volatile("int $0x80"
                   : "=a"(ret)
                   : "a"(SYS_SIGACTION), "b"(signum), "c"((uint32_t)handler), "d"(&old));
  if (ret < 0) {
    errno = -ret;
    return SIG_ERR;
  }
  return (sighandler_t)old;
}

int raise(int sig) { return kill(getpid(), sig); }

#endif

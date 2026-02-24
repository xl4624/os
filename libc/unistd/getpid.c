#include <unistd.h>

#if defined(__is_libk)

int getpid(void) { return 0; }

#elif defined(__is_libc)

#include <sys/syscall.h>

int getpid(void) {
  int pid;
  asm volatile("int $0x80" : "=a"(pid) : "a"(SYS_GETPID));
  return pid;
}

#endif

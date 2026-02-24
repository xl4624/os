#include <unistd.h>

#if defined(__is_libk)

int fork(void) { return -1; }

#elif defined(__is_libc)

#include <sys/syscall.h>

int fork(void) {
  int pid;
  asm volatile("int $0x80" : "=a"(pid) : "a"(SYS_FORK));
  return pid;
}

#endif

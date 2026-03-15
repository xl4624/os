#include <unistd.h>

#ifdef __is_libk

int fork(void) { return -1; }

#elif defined(__is_libc)

#include <stdint.h>
#include <sys/syscall.h>

int fork(void) {
  int32_t pid;
  __asm__ volatile("int $0x80" : "=a"(pid) : "a"(SYS_FORK));
  return pid;
}

#endif

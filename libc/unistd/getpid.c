#include <unistd.h>

#ifdef __is_libk

int getpid(void) { return 0; }

#elif defined(__is_libc)

#include <stdint.h>
#include <sys/syscall.h>

int getpid(void) {
  int32_t pid;
  __asm__ volatile("int $0x80" : "=a"(pid) : "a"(SYS_GETPID));
  return pid;
}

#endif

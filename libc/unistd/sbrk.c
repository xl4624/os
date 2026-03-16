#include <unistd.h>

#ifdef __is_libk

void* sbrk(int increment) {
  (void)increment;
  return (void*)-1;
}

#elif defined(__is_libc)

#include <errno.h>
#include <stdint.h>
#include <sys/syscall.h>

void* sbrk(int increment) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_SBRK), "b"(increment));
  if (ret < 0) {
    errno = -ret;
    return (void*)-1;
  }
  return (void*)ret;
}

#endif

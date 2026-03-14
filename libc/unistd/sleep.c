#include <unistd.h>

#if defined(__is_libk)

unsigned int sleep(unsigned int seconds) {
  (void)seconds;
  return 0;
}

#elif defined(__is_libc)

#include <stdint.h>
#include <sys/syscall.h>

unsigned int sleep(unsigned int seconds) {
  uint32_t ms = (uint32_t)seconds * 1000u;
  int32_t ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_SLEEP), "b"(ms));
  (void)ret;
  return 0;
}

#endif

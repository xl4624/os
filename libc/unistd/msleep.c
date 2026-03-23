#include <unistd.h>

#ifdef __is_libk

void msleep(unsigned int ms) { (void)ms; }

#else /* __is_libc */

#include <stdint.h>
#include <sys/syscall.h>

void msleep(unsigned int ms) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_SLEEP), "b"(ms));
  (void)ret;
}

#endif

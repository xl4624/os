#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

unsigned long long getticks(void) {
  uint32_t lo;
  uint32_t hi;
  __asm__ volatile("int $0x80" : "=a"(lo), "=d"(hi) : "a"(SYS_GETTICKS));
  return ((uint64_t)hi << 32) | lo;
}

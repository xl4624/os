#include <sys/syscall.h>
#include <unistd.h>

unsigned long long getticks(void) {
  unsigned int lo, hi;
  asm volatile("int $0x80" : "=a"(lo), "=d"(hi) : "a"(SYS_GETTICKS));
  return ((unsigned long long)hi << 32) | lo;
}

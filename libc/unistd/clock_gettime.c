#include <stdint.h>
#include <sys/syscall.h>
#include <time.h>

int clock_gettime(clockid_t clk_id, struct timespec* tp) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_CLOCK_GETTIME), "b"(clk_id), "c"(tp));
  return __syscall_ret(ret);
}

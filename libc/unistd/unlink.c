#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

int unlink(const char* path) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_UNLINK), "b"(path));
  return __syscall_ret(ret);
}

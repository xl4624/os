#include <stdint.h>
#include <sys/syscall.h>

int rename(const char* oldpath, const char* newpath) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_RENAME), "b"(oldpath), "c"(newpath));
  return __syscall_ret(ret);
}

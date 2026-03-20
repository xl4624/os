#include <dirent.h>
#include <stdint.h>
#include <sys/syscall.h>

int getdents(const char* path, struct dirent* buf, unsigned int count) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETDENTS), "b"(path), "c"(buf), "d"(count));
  return __syscall_ret(ret);
}

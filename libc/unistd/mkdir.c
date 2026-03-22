#include <stdint.h>
#include <sys/stat.h>
#include <sys/syscall.h>

int mkdir(const char* path, mode_t mode) {
  (void)mode;
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_MKDIR), "b"(path));
  return __syscall_ret(ret);
}

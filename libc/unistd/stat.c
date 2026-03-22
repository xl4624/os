#include <stdint.h>
#include <sys/stat.h>
#include <sys/syscall.h>

int stat(const char* path, struct stat* buf) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_STAT), "b"(path), "c"(buf));
  return __syscall_ret(ret);
}

int fstat(int fd, struct stat* buf) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_FSTAT), "b"(fd), "c"(buf));
  return __syscall_ret(ret);
}

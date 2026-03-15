#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

int lseek(int fd, int offset, int whence) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_LSEEK), "b"(fd), "c"(offset), "d"(whence));
  return ret;
}

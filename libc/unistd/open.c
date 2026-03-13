#include <sys/syscall.h>
#include <unistd.h>

int open(const char* path) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_OPEN), "b"(path));
  return ret;
}

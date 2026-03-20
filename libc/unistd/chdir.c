#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

int chdir(const char* path) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_CHDIR), "b"(path));
  return __syscall_ret(ret);
}

int getcwd_impl(char* buf, unsigned int size) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETCWD), "b"(buf), "c"(size));
  return __syscall_ret(ret);
}

char* getcwd(char* buf, unsigned int size) {
  if (getcwd_impl(buf, size) < 0) {
    return (char*)0;
  }
  return buf;
}

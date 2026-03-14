#include <unistd.h>

#if defined(__is_libk)

int exec(const char* name) {
  (void)name;
  return -1;
}

#elif defined(__is_libc)

#include <stdint.h>
#include <sys/syscall.h>

int exec(const char* name) {
  int32_t ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_EXEC), "b"(name));
  return ret;
}

#endif

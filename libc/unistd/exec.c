#include <unistd.h>

#if defined(__is_libk)

int exec(const char* path, char* const argv[]) {
  (void)path;
  (void)argv;
  return -1;
}

#elif defined(__is_libc)

#include <stdint.h>
#include <sys/syscall.h>

int exec(const char* path, char* const argv[]) {
  int32_t ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_EXEC), "b"(path), "c"(argv));
  return ret;
}

#endif

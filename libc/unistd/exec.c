#include <unistd.h>

#ifdef __is_libk

int exec(const char* path, char* const argv[]) {
  (void)path;
  (void)argv;
  return -1;
}

#else /* __is_libc */

#include <stdint.h>
#include <sys/syscall.h>

int exec(const char* path, char* const argv[]) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_EXEC), "b"(path), "c"(argv));
  return __syscall_ret(ret);
}

#endif

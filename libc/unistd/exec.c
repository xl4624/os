#include <unistd.h>

#ifdef __is_libk

int exec(const char* path, char* const argv[], char* const envp[]) {
  (void)path;
  (void)argv;
  (void)envp;
  return -1;
}

#else /* __is_libc */

#include <stdint.h>
#include <sys/syscall.h>

int exec(const char* path, char* const argv[], char* const envp[]) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_EXEC), "b"(path), "c"(argv), "d"(envp));
  return __syscall_ret(ret);
}

#endif

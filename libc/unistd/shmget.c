#include <sys/shm.h>

#ifdef __is_libk

int shmget(unsigned int size) {
  (void)size;
  return -1;
}

#else /* __is_libc */

#include <stdint.h>
#include <sys/syscall.h>

int shmget(unsigned int size) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_SHMGET), "b"(size));
  return __syscall_ret(ret);
}

#endif

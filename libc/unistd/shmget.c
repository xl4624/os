#include <sys/shm.h>

#if defined(__is_libk)

int shmget(unsigned int size) {
  (void)size;
  return -1;
}

#elif defined(__is_libc)

#include <sys/syscall.h>

int shmget(unsigned int size) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_SHMGET), "b"(size));
  return ret;
}

#endif

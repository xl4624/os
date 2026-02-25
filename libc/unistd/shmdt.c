#include <sys/shm.h>

#if defined(__is_libk)

int shmdt(void* vaddr, unsigned int size) {
  (void)vaddr;
  (void)size;
  return -1;
}

#elif defined(__is_libc)

#include <sys/syscall.h>

int shmdt(void* vaddr, unsigned int size) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_SHMDT), "b"(vaddr), "c"(size));
  return ret;
}

#endif

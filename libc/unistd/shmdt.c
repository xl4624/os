#include <sys/shm.h>

#ifdef __is_libk

int shmdt(void* vaddr, unsigned int size) {
  (void)vaddr;
  (void)size;
  return -1;
}

#elif defined(__is_libc)

#include <stdint.h>
#include <sys/syscall.h>

int shmdt(void* vaddr, unsigned int size) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_SHMDT), "b"(vaddr), "c"(size));
  return __syscall_ret(ret);
}

#endif

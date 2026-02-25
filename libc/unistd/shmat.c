#include <sys/shm.h>

#if defined(__is_libk)

int shmat(int shmid, void* vaddr) {
  (void)shmid;
  (void)vaddr;
  return -1;
}

#elif defined(__is_libc)

#include <sys/syscall.h>

int shmat(int shmid, void* vaddr) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_SHMAT), "b"(shmid), "c"(vaddr));
  return ret;
}

#endif

#include <stdint.h>
#include <sys/mount.h>
#include <sys/syscall.h>

int mount(const char* target, const char* fstype) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_MOUNT), "b"(target), "c"(fstype));
  return __syscall_ret(ret);
}

int umount(const char* target) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_UMOUNT), "b"(target));
  return __syscall_ret(ret);
}

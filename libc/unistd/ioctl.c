#ifdef __is_libc

#include <stdarg.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

int ioctl(int fd, unsigned long request, ...) {
  va_list ap;
  va_start(ap, request);
  void* arg = va_arg(ap, void*);
  va_end(ap);
  int32_t ret;
  __asm__ volatile("int $0x80"
                   : "=a"(ret)
                   : "a"(SYS_IOCTL), "b"(fd), "c"(request), "d"(arg)
                   : "memory");
  return __syscall_ret(ret);
}

#endif

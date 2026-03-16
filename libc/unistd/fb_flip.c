#include <stdint.h>
#include <sys/fb.h>
#include <sys/syscall.h>

#ifdef __is_libk
int fb_flip(const void* src, unsigned int w, unsigned int h) {
  (void)src;
  (void)w;
  (void)h;
  return -1;
}
#elif defined(__is_libc)
int fb_flip(const void* src, unsigned int w, unsigned int h) {
  int32_t ret;
  __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_FB_FLIP), "b"(src), "c"(w), "d"(h));
  return __syscall_ret(ret);
}
#endif

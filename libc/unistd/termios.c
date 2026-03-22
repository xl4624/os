typedef int _iso_c_forbids_empty_tu;

#ifdef __is_libc

#include <sys/ioctl.h>
#include <termios.h>

int tcgetattr(int fd, struct termios* t) { return ioctl(fd, TCGETS, t); }

int tcsetattr(int fd, int action, const struct termios* t) {
  (void)action;
  return ioctl(fd, TCSETS, (void*)t);
}

#endif

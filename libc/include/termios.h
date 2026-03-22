#ifndef _TERMIOS_H
#define _TERMIOS_H

#include <stdint.h>

#define NCCS 20

typedef uint32_t tcflag_t;
typedef uint8_t cc_t;

struct termios {
  tcflag_t c_iflag;
  tcflag_t c_oflag;
  tcflag_t c_cflag;
  tcflag_t c_lflag;
  cc_t c_cc[NCCS];
};

/* c_lflag bits */
#define ISIG 0x0001
#define ICANON 0x0002
#define ECHO 0x0008
#define ECHOE 0x0010

/* c_cc indices */
#define VINTR 0
#define VEOF 4
#define VTIME 5
#define VMIN 6

/* tcsetattr actions */
#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2

int tcgetattr(int fd, struct termios* t);
int tcsetattr(int fd, int action, const struct termios* t);

#endif

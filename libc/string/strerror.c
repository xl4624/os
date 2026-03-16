#include <string.h>

char* strerror(int errnum) {
  (void)errnum;
  return "error";
}

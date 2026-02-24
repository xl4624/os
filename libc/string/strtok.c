#include <string.h>

static char* strtok_state = NULL;

char* strtok(char* __restrict__ str, const char* __restrict__ delim) {
  return strtok_r(str, delim, &strtok_state);
}

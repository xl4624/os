#include <string.h>

static char* strtok_state = NULL;

char* strtok(char* restrict str, const char* restrict delim) {
  return strtok_r(str, delim, &strtok_state);
}

#include <string.h>

char* strcpy(char* dest, const char* src) {
  char* start = dest;
  while (*src != '\0') {
    *dest = *src;
    ++dest;
    ++src;
  }
  *dest = '\0';
  return start;
}

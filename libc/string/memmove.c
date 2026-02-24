#include <string.h>

void* memmove(void* dest, const void* src, size_t n) {
  unsigned char* d = (unsigned char*)dest;
  const unsigned char* s = (const unsigned char*)src;

  // Choose direction by checking where src and dest are relative to each
  // other to avoid overwriting
  if (d > s) {
    for (size_t i = 0; i < n; ++i) {
      d[i] = s[i];
    }
  } else {
    for (size_t i = n; i-- > 0;) {
      d[i] = s[i];
    }
  }
  return dest;
}

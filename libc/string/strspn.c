#include <string.h>

size_t strspn(const char* s, const char* accept) {
  size_t count = 0;

  for (const char* p = s; *p != '\0'; ++p) {
    for (const char* a = accept;; ++a) {
      if (*a == '\0') {
        return count;
      }
      if (*p == *a) {
        break;
      }
    }
    ++count;
  }

  return count;
}

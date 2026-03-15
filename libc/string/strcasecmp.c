#include <strings.h>

static int to_lower(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

int strcasecmp(const char* s1, const char* s2) {
  while (*s1 && *s2) {
    int diff = to_lower((unsigned char)*s1) - to_lower((unsigned char)*s2);
    if (diff != 0) {
      return diff;
    }
    ++s1;
    ++s2;
  }
  return to_lower((unsigned char)*s1) - to_lower((unsigned char)*s2);
}

int strncasecmp(const char* s1, const char* s2, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    int diff = to_lower((unsigned char)s1[i]) - to_lower((unsigned char)s2[i]);
    if (diff != 0) {
      return diff;
    }
    if (s1[i] == '\0') {
      return 0;
    }
  }
  return 0;
}

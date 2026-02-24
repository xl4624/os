#include <string.h>

char* strtok_r(char* __restrict__ str, const char* __restrict__ delim,
               char** __restrict__ saveptr) {
  char* token;

  if (str != NULL) {
    *saveptr = str;
  }

  token = *saveptr;

  if (token == NULL) {
    return NULL;
  }

  token += strspn(token, delim);

  if (*token == '\0') {
    *saveptr = NULL;
    return NULL;
  }

  char* end = token + strcspn(token, delim);

  if (*end == '\0') {
    *saveptr = NULL;
  } else {
    *end = '\0';
    *saveptr = end + 1;
  }

  return token;
}

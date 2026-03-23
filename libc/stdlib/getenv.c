#include <stdlib.h>
#include <string.h>

char* getenv(const char* name) {
  if (!environ || !name) {
    return (char*)0;
  }
  const size_t len = strlen(name);
  for (char** ep = environ; *ep; ++ep) {
    if (strncmp(*ep, name, len) == 0 && (*ep)[len] == '=') {
      return *ep + len + 1;
    }
  }
  return (char*)0;
}

#include <errno.h>
#include <stdlib.h>
#include <string.h>

// Tracks whether we malloc'd the environ array itself (vs. pointing at the
// initial envp on the stack set by crt0.S).
static int environ_is_ours = 0;

static int environ_count(void) {
  int n = 0;
  if (environ) {
    while (environ[n]) {
      ++n;
    }
  }
  return n;
}

int setenv(const char* name, const char* value, int overwrite) {
  if (!name || *name == '\0' || strchr(name, '=')) {
    errno = EINVAL;
    return -1;
  }

  const size_t nlen = strlen(name);
  const size_t vlen = strlen(value);
  const int n = environ_count();

  // Check if NAME already exists.
  for (int i = 0; i < n; ++i) {
    if (strncmp(environ[i], name, nlen) == 0 && environ[i][nlen] == '=') {
      if (!overwrite) {
        return 0;
      }
      char* s = (char*)malloc(nlen + 1 + vlen + 1);
      if (!s) {
        errno = ENOMEM;
        return -1;
      }
      memcpy(s, name, nlen);
      s[nlen] = '=';
      strcpy(s + nlen + 1, value);
      // Ensure we own the array before writing into it.
      if (!environ_is_ours) {
        char** copy = (char**)malloc((size_t)(n + 1) * sizeof(char*));
        if (!copy) {
          free(s);
          errno = ENOMEM;
          return -1;
        }
        for (int j = 0; j < n; ++j) {
          copy[j] = environ[j];
        }
        copy[n] = (char*)0;
        environ = copy;
        environ_is_ours = 1;
      }
      environ[i] = s;
      return 0;
    }
  }

  // Name not found -- append a new entry.
  char** new_env;
  if (!environ_is_ours) {
    new_env = (char**)malloc((size_t)(n + 2) * sizeof(char*));
    if (!new_env) {
      errno = ENOMEM;
      return -1;
    }
    for (int i = 0; i < n; ++i) {
      new_env[i] = environ[i];
    }
  } else {
    new_env = (char**)realloc((void*)environ, (size_t)(n + 2) * sizeof(char*));
    if (!new_env) {
      errno = ENOMEM;
      return -1;
    }
  }

  char* s = (char*)malloc(nlen + 1 + vlen + 1);
  if (!s) {
    if (!environ_is_ours) {
      free((void*)new_env);
    }
    errno = ENOMEM;
    return -1;
  }
  memcpy(s, name, nlen);
  s[nlen] = '=';
  strcpy(s + nlen + 1, value);
  new_env[n] = s;
  new_env[n + 1] = (char*)0;
  environ = new_env;
  environ_is_ours = 1;
  return 0;
}

int unsetenv(const char* name) {
  if (!name || *name == '\0' || strchr(name, '=')) {
    errno = EINVAL;
    return -1;
  }
  if (!environ) {
    return 0;
  }

  const size_t nlen = strlen(name);
  int n = environ_count();

  for (int i = 0; i < n; ++i) {
    if (strncmp(environ[i], name, nlen) == 0 && environ[i][nlen] == '=') {
      // Ensure we own the array before shifting entries.
      if (!environ_is_ours) {
        char** copy = (char**)malloc((size_t)(n + 1) * sizeof(char*));
        if (!copy) {
          errno = ENOMEM;
          return -1;
        }
        for (int j = 0; j < n; ++j) {
          copy[j] = environ[j];
        }
        copy[n] = (char*)0;
        environ = copy;
        environ_is_ours = 1;
      }
      // Shift remaining entries down to fill the gap.
      for (int j = i; j < n - 1; ++j) {
        environ[j] = environ[j + 1];
      }
      environ[n - 1] = (char*)0;
      return 0;
    }
  }
  return 0;
}

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

static int char_to_digit(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'z') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'Z') {
    return c - 'A' + 10;
  }
  return -1;
}

unsigned long strtoul(const char* nptr, char** endptr, int base) {
  const char* s = nptr;

  while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' || *s == '\f' || *s == '\v') {
    ++s;
  }

  int negative = 0;
  if (*s == '+') {
    ++s;
  } else if (*s == '-') {
    negative = 1;
    ++s;
  }

  if (base == 0) {
    if (*s == '0') {
      ++s;
      if (*s == 'x' || *s == 'X') {
        base = 16;
        ++s;
      } else {
        base = 8;
      }
    } else {
      base = 10;
    }
  } else if (base == 16) {
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
      s += 2;
    }
  }

  unsigned long result = 0;
  int any = 0;
  unsigned long cutoff = ULONG_MAX / (unsigned long)base;
  int cutlim = (int)(ULONG_MAX % (unsigned long)base);

  for (;; ++s) {
    int d = char_to_digit(*s);
    if (d < 0 || d >= base) {
      break;
    }
    any = 1;
    if (result > cutoff || (result == cutoff && d > cutlim)) {
      result = ULONG_MAX;
      errno = ERANGE;
    } else {
      result = (result * (unsigned long)base) + (unsigned long)d;
    }
  }

  if (endptr) {
    *endptr = (char*)(any ? s : nptr);
  }
  if (negative) {
    result = (unsigned long)(-(long)result);
  }
  return result;
}

long strtol(const char* nptr, char** endptr, int base) {
  const char* s = nptr;

  while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' || *s == '\f' || *s == '\v') {
    ++s;
  }

  int negative = 0;
  if (*s == '+') {
    ++s;
  } else if (*s == '-') {
    negative = 1;
    ++s;
  }

  if (base == 0) {
    if (*s == '0') {
      ++s;
      if (*s == 'x' || *s == 'X') {
        base = 16;
        ++s;
      } else {
        base = 8;
      }
    } else {
      base = 10;
    }
  } else if (base == 16) {
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
      s += 2;
    }
  }

  unsigned long result = 0;
  int any = 0;
  unsigned long cutoff = negative ? (unsigned long)-(LONG_MIN + LONG_MAX) + (unsigned long)LONG_MAX
                                  : (unsigned long)LONG_MAX;
  int cutlim = (int)(cutoff % (unsigned long)base);
  cutoff /= (unsigned long)base;

  for (;; ++s) {
    int d = char_to_digit(*s);
    if (d < 0 || d >= base) {
      break;
    }
    any = 1;
    if (result > cutoff || (result == cutoff && (unsigned long)d > (unsigned long)cutlim)) {
      result = negative ? (unsigned long)LONG_MIN : (unsigned long)LONG_MAX;
      errno = ERANGE;
    } else {
      result = (result * (unsigned long)base) + (unsigned long)d;
    }
  }

  if (endptr) {
    *endptr = (char*)(any ? s : nptr);
  }
  if (negative) {
    return -(long)result;
  }
  return (long)result;
}

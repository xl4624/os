#include <ctype.h>
#include <stdlib.h>

double atof(const char* s) {
  while (*s == ' ' || *s == '\t') {
    ++s;
  }

  double sign = 1.0;
  if (*s == '-') {
    sign = -1.0;
    ++s;
  } else if (*s == '+') {
    ++s;
  }

  double result = 0.0;
  while (*s >= '0' && *s <= '9') {
    result = (result * 10.0) + (*s++ - '0');
  }

  if (*s == '.') {
    ++s;
    double frac = 0.1;
    while (*s >= '0' && *s <= '9') {
      result += (*s++ - '0') * frac;
      frac *= 0.1;
    }
  }

  if (*s == 'e' || *s == 'E') {
    ++s;
    int esign = 1;
    if (*s == '-') {
      esign = -1;
      ++s;
    } else if (*s == '+') {
      ++s;
    }
    int exp = 0;
    while (*s >= '0' && *s <= '9') {
      exp = (exp * 10) + (*s++ - '0');
    }
    double base = 10.0;
    while (exp > 0) {
      if (exp & 1) {
        result = esign > 0 ? result * base : result / base;
      }
      base *= base;
      exp >>= 1;
    }
  }

  return sign * result;
}

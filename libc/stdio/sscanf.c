#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int is_space(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

static int to_digit(char c, int base) {
  int d;
  if (isdigit(c)) {
    d = c - '0';
  } else if (islower(c)) {
    d = c - 'a' + 10;
  } else if (isupper(c)) {
    d = c - 'A' + 10;
  } else {
    return -1;
  }
  return d < base ? d : -1;
}

int sscanf(const char* str, const char* format, ...) {
  va_list ap;
  va_start(ap, format);

  int matched = 0;
  const char* s = str;

  while (*format) {
    if (is_space(*format)) {
      while (is_space(*format)) {
        ++format;
      }
      while (is_space(*s)) {
        ++s;
      }
      continue;
    }

    if (*format != '%') {
      if (*s != *format) {
        break;
      }
      ++s;
      ++format;
      continue;
    }

    ++format;

    // check for suppression
    int suppress = 0;
    if (*format == '*') {
      suppress = 1;
      ++format;
    }

    // parse width
    int width = 0;
    while (*format >= '0' && *format <= '9') {
      width = width * 10 + (*format - '0');
      ++format;
    }
    if (width == 0) {
      width = -1;  // unlimited
    }

    switch (*format) {
      case 'd': {
        while (is_space(*s)) {
          ++s;
        }
        int negative = 0;
        if (*s == '-') {
          negative = 1;
          ++s;
        } else if (*s == '+') {
          ++s;
        }
        if (!(*s >= '0' && *s <= '9')) {
          goto done;
        }
        long val = 0;
        int chars = 0;
        while (*s >= '0' && *s <= '9' && (width < 0 || chars < width)) {
          val = val * 10 + (*s - '0');
          ++s;
          ++chars;
        }
        if (negative) {
          val = -val;
        }
        if (!suppress) {
          *va_arg(ap, int*) = (int)val;
          ++matched;
        }
        break;
      }
      case 'x':
      case 'X': {
        while (is_space(*s)) {
          ++s;
        }
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
          s += 2;
        }
        int d = to_digit(*s, 16);
        if (d < 0) {
          goto done;
        }
        unsigned long val = 0;
        int chars = 0;
        while ((d = to_digit(*s, 16)) >= 0 && (width < 0 || chars < width)) {
          val = val * 16 + (unsigned long)d;
          ++s;
          ++chars;
        }
        if (!suppress) {
          *va_arg(ap, unsigned int*) = (unsigned int)val;
          ++matched;
        }
        break;
      }
      case 's': {
        while (is_space(*s)) {
          ++s;
        }
        if (!*s) {
          goto done;
        }
        if (!suppress) {
          char* dest = va_arg(ap, char*);
          int chars = 0;
          while (*s && !is_space(*s) && (width < 0 || chars < width)) {
            *dest++ = *s++;
            ++chars;
          }
          *dest = '\0';
        } else {
          int chars = 0;
          while (*s && !is_space(*s) && (width < 0 || chars < width)) {
            ++s;
            ++chars;
          }
        }
        ++matched;
        break;
      }
      case 'c': {
        if (!*s) {
          goto done;
        }
        if (!suppress) {
          *va_arg(ap, char*) = *s;
          ++matched;
        }
        ++s;
        break;
      }
      case '%': {
        if (*s != '%') {
          goto done;
        }
        ++s;
        break;
      }
      default:
        goto done;
    }
    ++format;
  }

done:
  va_end(ap);
  return matched;
}

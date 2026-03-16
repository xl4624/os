/*
 * printf() implementation adapted from WeensyOS
 *
 * Copyright (C) 2018-2020 University College London
 * Copyright (C) 2006-2016 Eddie Kohler
 * Copyright (C) 2005 Regents of the University of California
 * Copyright (C) 1997 Massachusetts Institute of Technology
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose and without fee or royalty is
 * hereby granted, provided that the full text of this NOTICE appears on
 * ALL copies of the software and documentation or portions thereof,
 * including modifications, that you make.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY OF EXAMPLE,
 * BUT NOT LIMITATION, COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS OR
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR
 * THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY
 * THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS. COPYRIGHT
 * HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE OR
 * DOCUMENTATION.
 *
 * The name and trademarks of copyright holders may NOT be used in
 * advertising or publicity pertaining to the software without specific,
 * written prior permission. Title to copyright in this software and any
 * associated documentation will at all times remain with copyright
 * holders. See the file AUTHORS which should have accompanied this software
 * for a list of all copyright holders.
 *
 * This file may be derived from previously copyrighted software. This
 * copyright applies only to those changes made by the copyright
 * holders listed in the AUTHORS file. The rest of this file is covered by
 * the copyright notices, if any, listed below.
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef void (*emit_fn)(char c, void* ctx);

// clang-format off
enum {
FLAG_ALT =         (1 << 0),
FLAG_ZERO =        (1 << 1),
FLAG_LEFTJUSTIFY = (1 << 2),
FLAG_SPACE =       (1 << 3),
FLAG_PLUS =        (1 << 4)
};
static const char flag_chars[] = "#0- +";

enum {
FLAG_NUMERIC =     (1 << 5),
FLAG_SIGNED =      (1 << 6),
FLAG_NEGATIVE =    (1 << 7),
FLAG_ALT2 =        (1 << 8)
};
// clang-format on

static char* fill_numbuf(char* numbuf_end, unsigned long val, unsigned int base) {
  static const char upper_digits[] = "0123456789ABCDEF";
  static const char lower_digits[] = "0123456789abcdef";

  const char* digits = upper_digits;
  if ((int)base < 0) {
    digits = lower_digits;
    base = (unsigned int)(-(int)base);
  }

  *--numbuf_end = '\0';
  do {
    *--numbuf_end = digits[val % base];
    val /= base;
  } while (val != 0);
  return numbuf_end;
}

#define NUMBUF_SIZE 24

// NOLINTNEXTLINE(misc-const-correctness)
static int vformat(emit_fn emit, void* ctx, const char* __restrict__ format, va_list ap) {
  char numbuf[NUMBUF_SIZE];
  int count = 0;

  for (; *format; ++format) {
    if (*format != '%') {
      emit(*format, ctx);
      ++count;
      continue;
    }

    // process flags
    int flags = 0;
    for (++format; *format; ++format) {
      const char* flagc = strchr(flag_chars, *format);
      if (!flagc) {
        break;
      }
      flags |= 1 << (flagc - flag_chars);
    }

    // process width
    int width = -1;
    if (*format >= '1' && *format <= '9') {
      for (width = 0; *format >= '0' && *format <= '9'; ++format) {
        width = (10 * width) + *format - '0';
      }
    } else if (*format == '*') {
      width = va_arg(ap, int);
      ++format;
    }

    // process precision
    int precision = -1;
    if (*format == '.') {
      ++format;
      if (*format >= '0' && *format <= '9') {
        for (precision = 0; *format >= '0' && *format <= '9'; ++format) {
          precision = (10 * precision) + *format - '0';
        }
      } else if (*format == '*') {
        precision = va_arg(ap, int);
        ++format;
        ;
      }
      if (precision < 0) {
        precision = 0;
      }
    }

    // process main conversion character
    int base = 10;
    unsigned long num = 0;
    int length = 0;
    char* data = "";
  again:
    switch (*format) {
      case 'l':
      case 'z':
        length = 1;
        ++format;
        goto again;
      case 'd':
      case 'i': {
        long x = length ? va_arg(ap, long) : va_arg(ap, int);
        int negative = x < 0 ? FLAG_NEGATIVE : 0;
        num = negative ? (unsigned long)(-x) : (unsigned long)x;
        flags |= FLAG_NUMERIC | FLAG_SIGNED | negative;
        break;
      }
      case 'u':
      format_unsigned:
        num = length ? va_arg(ap, unsigned long) : va_arg(ap, unsigned);
        flags |= FLAG_NUMERIC;
        break;
      case 'x':
        base = -16;
        goto format_unsigned;
      case 'X':
        base = 16;
        goto format_unsigned;
      case 'p':
        num = (uintptr_t)va_arg(ap, void*);
        base = -16;
        flags |= FLAG_ALT | FLAG_ALT2 | FLAG_NUMERIC;
        break;
      case 's':
        data = va_arg(ap, char*);
        break;
      case 'c':
        data = numbuf;
        // char always promoted to int in varargs
        numbuf[0] = (char)va_arg(ap, int);
        numbuf[1] = '\0';
        break;
      default:
        data = numbuf;
        numbuf[0] = (char)(*format ? *format : '%');
        numbuf[1] = '\0';
        if (!*format) {
          --format;
        }
        break;
    }

    if (flags & FLAG_NUMERIC) {
      data = fill_numbuf(numbuf + NUMBUF_SIZE, num, (unsigned int)base);
    }

    const char* prefix = "";
    if ((flags & FLAG_NUMERIC) && (flags & FLAG_SIGNED)) {
      if (flags & FLAG_NEGATIVE) {
        prefix = "-";
      } else if (flags & FLAG_PLUS) {
        prefix = "+";
      } else if (flags & FLAG_SPACE) {
        prefix = " ";
      }
    } else if ((flags & FLAG_NUMERIC) && (flags & FLAG_ALT) && (base == 16 || base == -16) &&
               (num || (flags & FLAG_ALT2))) {
      prefix = (base == -16 ? "0x" : "0X");
    }

    int len = 0;
    if (precision >= 0 && !(flags & FLAG_NUMERIC)) {
      len = (int)strnlen(data, (size_t)precision);
    } else if (precision == 0 && num == 0) {
      len = 0;
    } else {
      len = (int)strlen(data);
    }
    int zeros = 0;
    if ((flags & FLAG_NUMERIC) && precision >= 0) {
      zeros = precision > len ? precision - len : 0;
    } else if ((flags & FLAG_NUMERIC) && (flags & FLAG_ZERO) && !(flags & FLAG_LEFTJUSTIFY) &&
               len + (int)strlen(prefix) < width) {
      zeros = width - len - (int)strlen(prefix);
    } else {
      zeros = 0;
    }
    width -= len + zeros + (int)strlen(prefix);
    for (; !(flags & FLAG_LEFTJUSTIFY) && width > 0; --width) {
      emit(' ', ctx);
      ++count;
    }
    for (; *prefix; ++prefix) {
      emit(*prefix, ctx);
      ++count;
    }
    for (; zeros > 0; --zeros) {
      emit('0', ctx);
      ++count;
    }
    for (; len > 0; ++data, --len) {
      // clang-format off
      emit(*data, ctx); // NOLINT(clang-analyzer-core.CallAndMessage,clang-analyzer-security.ArrayBound)
      // clang-format on
      ++count;
    }
    for (; width > 0; --width) {
      emit(' ', ctx);
      ++count;
    }
  }
  return count;
}

// ==== putchar-based emitter (printf/vprintf) ====

static void emit_putchar(char c, void* ctx) {
  (void)ctx;
  putchar(c);
}

// NOLINTNEXTLINE(misc-const-correctness)
int vprintf(const char* __restrict__ format, va_list ap) {
  return vformat(emit_putchar, 0, format, ap);
}

int printf(const char* __restrict__ format, ...) {
  va_list ap;
  va_start(ap, format);
  int ret = vprintf(format, ap);
  va_end(ap);
  return ret;
}

// ==== buffer-based emitter (snprintf/sprintf/vsprintf) ====

struct buf_ctx {
  char* buf;
  size_t size;
  size_t pos;
};

static void emit_buf(char c, void* ctx) {
  struct buf_ctx* b = (struct buf_ctx*)ctx;
  if (b->pos + 1 < b->size) {
    b->buf[b->pos] = c;
  }
  b->pos++;
}

int vsnprintf(char* buf, size_t size, const char* __restrict__ format, va_list ap) {
  struct buf_ctx b;
  b.buf = buf;
  b.size = size;
  b.pos = 0;

  int ret = vformat(emit_buf, (void*)&b, format, ap);

  if (size > 0) {
    size_t end = b.pos < size ? b.pos : size - 1;
    buf[end] = '\0';
  }
  return ret;
}

int snprintf(char* buf, size_t size, const char* __restrict__ format, ...) {
  va_list ap;
  va_start(ap, format);
  int ret = vsnprintf(buf, size, format, ap);
  va_end(ap);
  return ret;
}

static void emit_buf_unlimited(char c, void* ctx) {
  char** p = (char**)ctx;
  *(*p)++ = c;
}

int vsprintf(char* buf, const char* __restrict__ format, va_list ap) {
  char* p = buf;
  int ret = vformat(emit_buf_unlimited, (void*)&p, format, ap);
  *p = '\0';
  return ret;
}

int sprintf(char* buf, const char* __restrict__ format, ...) {
  va_list ap;
  va_start(ap, format);
  int ret = vsprintf(buf, format, ap);
  va_end(ap);
  return ret;
}

#ifdef __is_libc

// ==== FILE descriptor-based emitter (vfprintf/fprintf) ====

#include <unistd.h>

#define FD_BUF_SIZE 64

struct fd_ctx {
  int fd;
  char buf[FD_BUF_SIZE];
  int len;
};

static void fd_ctx_flush(struct fd_ctx* ctx) {
  if (ctx->len > 0) {
    (void)write(ctx->fd, ctx->buf, (size_t)ctx->len);
    ctx->len = 0;
  }
}

static void emit_fd(char c, void* ctx_) {
  struct fd_ctx* ctx = (struct fd_ctx*)ctx_;
  ctx->buf[ctx->len++] = c;
  if (ctx->len == FD_BUF_SIZE) {
    fd_ctx_flush(ctx);
  }
}

// NOLINTNEXTLINE(misc-const-correctness)
int vfprintf(FILE* file, const char* __restrict__ format, va_list ap) {
  struct fd_ctx ctx = {file->fd, {0}, 0};
  int ret = vformat(emit_fd, (void*)&ctx, format, ap);
  fd_ctx_flush(&ctx);
  return ret;
}

int fprintf(FILE* file, const char* __restrict__ format, ...) {
  va_list ap;
  va_start(ap, format);
  int ret = vfprintf(file, format, ap);
  va_end(ap);
  return ret;
}

#endif /* !__is_libc */

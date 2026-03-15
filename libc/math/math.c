#include <math.h>

double fabs(double x) {
  double result;
  __asm__ volatile(
      "fldl %1\n\t"
      "fabs\n\t"
      "fstpl %0"
      : "=m"(result)
      : "m"(x)
      : "st");
  return result;
}

double sqrt(double x) {
  double result;
  __asm__ volatile(
      "fldl %1\n\t"
      "fsqrt\n\t"
      "fstpl %0"
      : "=m"(result)
      : "m"(x)
      : "st");
  return result;
}

double sin(double x) {
  double result;
  __asm__ volatile(
      "fldl %1\n\t"
      "fsin\n\t"
      "fstpl %0"
      : "=m"(result)
      : "m"(x)
      : "st");
  return result;
}

double cos(double x) {
  double result;
  __asm__ volatile(
      "fldl %1\n\t"
      "fcos\n\t"
      "fstpl %0"
      : "=m"(result)
      : "m"(x)
      : "st");
  return result;
}

// fpatan computes atan(st(1) / st(0)) and pops -- so load y first, then x.
double atan2(double y, double x) {
  double result;
  __asm__ volatile(
      "fldl %2\n\t"  // push y; st(0)=y
      "fldl %1\n\t"  // push x; st(0)=x, st(1)=y
      "fpatan\n\t"   // st(0) = atan(y/x), pops x
      "fstpl %0"
      : "=m"(result)
      : "m"(x), "m"(y)
      : "st", "st(1)");
  return result;
}

// Change x87 rounding control, apply frndint, restore.
// RC field is bits 10-11 of the control word (mask 0x0C00).
// RC=01 (0x0400) = round toward -inf (floor)
// RC=10 (0x0800) = round toward +inf (ceil)
// Use eax as intermediary because x86 has no mem-to-mem mov.

double floor(double x) {
  unsigned short cw_save;
  unsigned short cw_new;
  double result;
  __asm__ volatile(
      "fnstcw %1\n\t"
      "movw   %1, %%ax\n\t"
      "andw   $0xF3FF, %%ax\n\t"
      "orw    $0x0400, %%ax\n\t"
      "movw   %%ax, %2\n\t"
      "fldcw  %2\n\t"
      "fldl   %3\n\t"
      "frndint\n\t"
      "fstpl  %0\n\t"
      "fldcw  %1"
      : "=m"(result), "=m"(cw_save), "=m"(cw_new)
      : "m"(x)
      : "eax", "st");
  return result;
}

double ceil(double x) {
  unsigned short cw_save;
  unsigned short cw_new;
  double result;
  __asm__ volatile(
      "fnstcw %1\n\t"
      "movw   %1, %%ax\n\t"
      "andw   $0xF3FF, %%ax\n\t"
      "orw    $0x0800, %%ax\n\t"
      "movw   %%ax, %2\n\t"
      "fldcw  %2\n\t"
      "fldl   %3\n\t"
      "frndint\n\t"
      "fstpl  %0\n\t"
      "fldcw  %1"
      : "=m"(result), "=m"(cw_save), "=m"(cw_new)
      : "m"(x)
      : "eax", "st");
  return result;
}

// ln(x) = ln(2) * log2(x); fyl2x computes st(1)*log2(st(0)).
double log(double x) {
  double result;
  __asm__ volatile(
      "fldln2\n\t"   // push ln(2); st(0)=ln2
      "fldl %1\n\t"  // push x; st(0)=x, st(1)=ln2
      "fyl2x\n\t"    // st(0) = ln2 * log2(x) = ln(x), pops x
      "fstpl %0"
      : "=m"(result)
      : "m"(x)
      : "st", "st(1)");
  return result;
}

// pow(x, y) = 2^(y * log2(x))
// Split exponent t = n + f where n = floor(t), f in [0, 1).
// 2^t = 2^f * 2^n; compute 2^f via f2xm1, scale by 2^n via fscale.
double pow(double x, double y) {
  if (x == 0.0) {
    return 0.0;
  }
  if (x < 0.0) {
    return 0.0;  // undefined for non-integer exponent
  }

  // t = y * log2(x) via fyl2x
  double t;
  __asm__ volatile(
      "fldl %2\n\t"  // push y
      "fldl %1\n\t"  // push x; st(0)=x, st(1)=y
      "fyl2x\n\t"    // st(0) = y * log2(x) = t, pops
      "fstpl %0"
      : "=m"(t)
      : "m"(x), "m"(y)
      : "st", "st(1)");

  // n = floor(t), f = t - n (f is in [0, 1), within f2xm1's domain [-1, 1])
  double n = floor(t);
  double f = t - n;

  // 2^f via f2xm1: computes 2^st(0) - 1
  double two_f;
  __asm__ volatile(
      "fldl %1\n\t"
      "f2xm1\n\t"  // st(0) = 2^f - 1
      "fstpl %0"
      : "=m"(two_f)
      : "m"(f)
      : "st");
  two_f += 1.0;  // now two_f = 2^f

  // result = 2^f * 2^n via fscale (st(0) = st(0) * 2^floor(st(1)))
  double result;
  __asm__ volatile(
      "fldl %2\n\t"       // st(0) = n
      "fldl %1\n\t"       // st(0) = two_f, st(1) = n
      "fscale\n\t"        // st(0) = two_f * 2^n; st(1) = n
      "fstp %%st(1)\n\t"  // copy result over n and pop -> st(0) = result
      "fstpl %0"
      : "=m"(result)
      : "m"(two_f), "m"(n)
      : "st", "st(1)");
  return result;
}

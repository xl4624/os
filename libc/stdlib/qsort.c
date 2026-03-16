#include <stdlib.h>
#include <string.h>

static void swap(char* a, char* b, size_t size) {
  char tmp;
  for (size_t i = 0; i < size; i++) {
    tmp = a[i];
    a[i] = b[i];
    b[i] = tmp;
  }
}

void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*)) {
  if (nmemb <= 1) {
    return;
  }

  /* Iterative quicksort using an explicit stack */
  typedef struct {
    size_t lo;
    size_t hi;
  } Range;
  /* Max stack depth: log2(nmemb) + 1, 64 covers 2^63 elements */
  Range stack[64];
  int top = 0;

  stack[top].lo = 0;
  stack[top].hi = nmemb - 1;
  top++;

  char* arr = (char*)base;

  while (top > 0) {
    top--;
    size_t lo = stack[top].lo;
    size_t hi = stack[top].hi;

    if (lo >= hi) {
      continue;
    }

    /* Median-of-three pivot */
    size_t mid = lo + ((hi - lo) / 2);
    if (compar(arr + (lo * size), arr + (mid * size)) > 0) {
      swap(arr + (lo * size), arr + (mid * size), size);
    }
    if (compar(arr + (lo * size), arr + (hi * size)) > 0) {
      swap(arr + (lo * size), arr + (hi * size), size);
    }
    if (compar(arr + (mid * size), arr + (hi * size)) > 0) {
      swap(arr + (mid * size), arr + (hi * size), size);
    }

    /* Pivot is now at mid; move to hi-1 if possible */
    char* pivot = arr + mid * size;

    size_t i = lo;
    size_t j = hi;

    while (1) {
      while (compar(arr + (i * size), pivot) < 0) {
        i++;
      }
      while (j > lo && compar(arr + (j * size), pivot) > 0) {
        j--;
      }
      if (i >= j) {
        break;
      }
      swap(arr + (i * size), arr + (j * size), size);
      /* Update pivot pointer if it moved */
      if (arr + (i * size) == pivot) {
        pivot = arr + (j * size);
      } else if (arr + (j * size) == pivot) {
        pivot = arr + (i * size);
      }
      i++;
      if (j > 0) {
        j--;
      }
    }

    if (top + 2 < 64) {
      stack[top].lo = lo;
      stack[top].hi = (i > 0) ? i - 1 : 0;
      top++;
      stack[top].lo = i + 1;
      stack[top].hi = hi;
      top++;
    }
  }
}

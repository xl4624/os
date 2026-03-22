#ifndef _TIME_H
#define _TIME_H

#include <stdint.h>
#include <sys/cdefs.h>

typedef int32_t clockid_t;

struct timespec {
  int32_t tv_sec;
  int32_t tv_nsec;
};

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1

__BEGIN_DECLS

int clock_gettime(clockid_t clk_id, struct timespec* tp);

__END_DECLS

#endif /* _TIME_H */

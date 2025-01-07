#ifndef _ASSERT_H
#define _ASSERT_H

#include "sys/cdefs.h"

__BEGIN_DECLS

#ifdef NDEBUG

    #define assert(expr) ((void)0)

#else /* Not NDEBUG. */

void __assert_fail(const char *__assertion, const char *__file,
                   unsigned int __line, const char *__function);

    #define assert(expr)    \
        ((expr) ? ((void)0) \
                : __assert_fail(#expr, __FILE__, __LINE__, __func__))

#endif /* NDEBUG */

__END_DECLS

#endif /* _ASSERT_H */

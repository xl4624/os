#ifndef _SYS_PARAM_H
#define _SYS_PARAM_H

#include <sys/cdefs.h>

__BEGIN_DECLS

#define MAX(a, b)           \
    ({                      \
        typeof(a) _a = (a); \
        typeof(b) _b = (b); \
        _a > _b ? _a : _b;  \
    })

#define MIN(a, b)           \
    ({                      \
        typeof(a) _a = (a); \
        typeof(b) _b = (b); \
        _a < _b ? _a : _b;  \
    })

__END_DECLS

#endif /* _SYS_PARAM_H */

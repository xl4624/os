// __cxa_atexit is called by the compiler to register destructors for
// function-local statics. In a kernel that never exits, the destructors
// will never run, so we can simply ignore the registration.

#include <sys/cdefs.h>

__BEGIN_DECLS

// NOLINTNEXTLINE(bugprone-reserved-identifier)
int __cxa_atexit(void (* /*unused*/)(void*), void* /*unused*/, void* /*unused*/) { return 0; }

__END_DECLS

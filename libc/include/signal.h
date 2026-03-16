#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <sys/cdefs.h>

#define SIGINT 2
#define SIGILL 4
#define SIGABRT 6
#define SIGFPE 8
#define SIGSEGV 11
#define SIGTERM 15
#define NSIG 32

typedef void (*sighandler_t)(int);

#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIG_ERR ((sighandler_t) - 1)

__BEGIN_DECLS

int kill(int pid, int sig);
sighandler_t signal(int signum, sighandler_t handler);
int raise(int sig);

__END_DECLS

#endif /* _SIGNAL_H */

#include <stdio.h>

#ifdef __is_libc

static FILE kStdin = {0, 0, 0};
static FILE kStdout = {1, 0, 0};
static FILE kStderr = {2, 0, 0};

FILE* __libc_stdin = &kStdin;
FILE* __libc_stdout = &kStdout;
FILE* __libc_stderr = &kStderr;

#endif /* !__is_libc */

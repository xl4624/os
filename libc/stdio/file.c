#include <stdio.h>

#ifdef __is_libc

static FILE kStdin = {.fd = 0, .eof = 0, .err = 0};   // NOLINT(misc-non-copyable-objects)
static FILE kStdout = {.fd = 1, .eof = 0, .err = 0};  // NOLINT(misc-non-copyable-objects)
static FILE kStderr = {.fd = 2, .eof = 0, .err = 0};  // NOLINT(misc-non-copyable-objects)

FILE* __libc_stdin = &kStdin;
FILE* __libc_stdout = &kStdout;
FILE* __libc_stderr = &kStderr;

#endif /* !__is_libc */

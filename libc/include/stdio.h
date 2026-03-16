#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>
#include <stddef.h>
#include <sys/cdefs.h>

#define EOF (-1)

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#ifdef __is_libc

typedef struct {
  int fd;
  int eof;
  int err;
} FILE;

extern FILE* __libc_stdin;
extern FILE* __libc_stdout;
extern FILE* __libc_stderr;

#define stdin __libc_stdin
#define stdout __libc_stdout
#define stderr __libc_stderr

#endif /* !__is_libc */

__BEGIN_DECLS

int vprintf(const char* __restrict__ format,
            va_list ap);  // NOLINT(misc-const-correctness)
int printf(const char* __restrict__ format, ...);
int vsnprintf(char* buf, size_t size, const char* __restrict__ format, va_list ap);
int snprintf(char* buf, size_t size, const char* __restrict__ format, ...);
int vsprintf(char* buf, const char* __restrict__ format, va_list ap);
int sprintf(char* buf, const char* __restrict__ format, ...);
int getchar(void);
int putchar(int c);
int puts(const char* s);

int sscanf(const char* str, const char* format, ...);

#ifdef __is_libc
int vfprintf(FILE* file, const char* __restrict__ format, va_list ap);
int fprintf(FILE* file, const char* __restrict__ format, ...);

FILE* fopen(const char* path, const char* mode);
int fclose(FILE* file);
size_t fread(void* ptr, size_t size, size_t nmemb, FILE* file);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* file);
int fseek(FILE* file, long offset, int whence);
long ftell(FILE* file);
void rewind(FILE* file);
int fgetc(FILE* file);
int fputc(int c, FILE* file);
char* fgets(char* s, int n, FILE* file);
int fputs(const char* s, FILE* file);
int feof(FILE* file);
int ferror(FILE* file);
int fflush(FILE* file);
#endif /* !__is_libc */

int remove(const char* path);
int rename(const char* oldpath, const char* newpath);

__END_DECLS

#endif /* _STDIO_H */

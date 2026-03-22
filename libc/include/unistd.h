#ifndef _UNISTD_H
#define _UNISTD_H

#include <stddef.h>
#include <sys/cdefs.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

__BEGIN_DECLS

/* POSIX getopt -- full interface in <getopt.h> */
extern char* optarg;
extern int optind;
extern int opterr;
extern int optopt;
int getopt(int argc, char* const argv[], const char* optstring);

__attribute__((noreturn)) void _exit(int status);
int getpid(void);
int fork(void);
int waitpid(int pid, int* exit_code);
int open(const char* path, int flags, ...);
int exec(const char* path, char* const argv[]);
int write(int fd, const void* buf, size_t count);
int read(int fd, void* buf, size_t count);
int close(int fd);
int dup2(int oldfd, int newfd);
int pipe(int pipefd[2]);
unsigned int sleep(unsigned int seconds);
void msleep(unsigned int ms);
void* sbrk(int increment);
int lseek(int fd, int offset, int whence);
int chdir(const char* path);
char* getcwd(char* buf, unsigned int size);
int unlink(const char* path);
int fcntl(int fd, int cmd, ...);

__END_DECLS

#endif

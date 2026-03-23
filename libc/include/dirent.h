#ifndef _DIRENT_H
#define _DIRENT_H

#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

#define DT_UNKNOWN 0
#define DT_CHR 2
#define DT_DIR 4
#define DT_REG 8

struct dirent {
  char d_name[128]; /* entry name (not the full path) */
  uint8_t d_type;   /* DT_REG, DT_DIR, DT_CHR, or DT_UNKNOWN */
  uint32_t d_size;  /* file size in bytes (0 for directories and devices) */
};

/* Maximum entries buffered by opendir. */
#define DIR_BUF_SIZE 64

typedef struct {
  struct dirent entries[DIR_BUF_SIZE]; /* buffered entries from getdents */
  int count;                           /* total entries returned */
  int index;                           /* current read position */
} DIR;

/* Low-level syscall wrapper. */
int getdents(const char* path, struct dirent* buf, unsigned int count);

/* POSIX-like directory stream API. */
DIR* opendir(const char* path);
struct dirent* readdir(DIR* dirp);
int closedir(DIR* dirp);

__END_DECLS

#endif

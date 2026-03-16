#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include <sys/cdefs.h>
#include <sys/types.h>

struct stat {
  dev_t st_dev;
  ino_t st_ino;
  mode_t st_mode;
  nlink_t st_nlink;
  uid_t st_uid;
  gid_t st_gid;
  dev_t st_rdev;
  off_t st_size;
  blksize_t st_blksize;
  blkcnt_t st_blocks;
};

__BEGIN_DECLS

int stat(const char* path, struct stat* buf);
int fstat(int fd, struct stat* buf);
int mkdir(const char* path, mode_t mode);

__END_DECLS

#endif /* _SYS_STAT_H */

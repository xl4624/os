#ifndef _DIRENT_H
#define _DIRENT_H

#include <stdint.h>

#define DT_UNKNOWN 0
#define DT_CHR     2
#define DT_DIR     4
#define DT_REG     8

struct dirent {
  char d_name[128];  /* entry name (not the full path) */
  uint8_t d_type;    /* DT_REG, DT_DIR, DT_CHR, or DT_UNKNOWN */
  uint32_t d_size;   /* file size in bytes (0 for directories and devices) */
};

#endif

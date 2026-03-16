#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef __is_libc

int remove(const char* path) {
  (void)path;
  return -1;
}

int rename(const char* oldpath, const char* newpath) {
  (void)oldpath;
  (void)newpath;
  return -1;
}

#else /* __is_libc */

#include <fcntl.h>

FILE* fopen(const char* path, const char* mode) {
  if (path == NULL || mode == NULL) {
    return NULL;
  }

  int flags = 0;
  switch (mode[0]) {
    case 'r':
      flags = (mode[1] == '+') ? O_RDWR : O_RDONLY;
      break;
    case 'w':
      flags = (mode[1] == '+') ? (O_RDWR | O_CREAT | O_TRUNC) : (O_WRONLY | O_CREAT | O_TRUNC);
      break;
    case 'a':
      flags = (mode[1] == '+') ? (O_RDWR | O_CREAT | O_APPEND) : (O_WRONLY | O_CREAT | O_APPEND);
      break;
    default:
      return NULL;
  }

  int fd = open(path, flags, 0666);
  if (fd < 0) {
    return NULL;
  }

  FILE* f = (FILE*)malloc(sizeof(FILE));
  if (f == NULL) {
    close(fd);
    return NULL;
  }

  f->fd = fd;
  f->eof = 0;
  f->err = 0;
  return f;
}

int fclose(FILE* file) {
  if (file == NULL) {
    return -1;
  }
  int ret = close(file->fd);
  free(file);
  return ret;
}

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* file) {
  if (file == NULL || ptr == NULL || size == 0 || nmemb == 0) {
    return 0;
  }
  size_t total = size * nmemb;
  int n = read(file->fd, ptr, total);
  if (n < 0) {
    file->err = 1;
    return 0;
  }
  if (n == 0) {
    file->eof = 1;
    return 0;
  }
  return (size_t)n / size;
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* file) {
  if (size == 0 || nmemb == 0) {
    return 0;
  }
  size_t total = size * nmemb;
  int n = write(file->fd, ptr, total);
  if (n < 0) {
    file->err = 1;
    return 0;
  }
  return (size_t)n / size;
}

int fseek(FILE* file, long offset, int whence) {
  if (file == NULL) {
    return -1;
  }
  file->eof = 0;
  int ret = lseek(file->fd, (int)offset, whence);
  if (ret < 0) {
    file->err = 1;
    return -1;
  }
  return 0;
}

long ftell(FILE* file) {
  if (file == NULL) {
    return -1;
  }
  return (long)lseek(file->fd, 0, SEEK_CUR);
}

void rewind(FILE* file) {
  if (file == NULL) {
    return;
  }
  (void)lseek(file->fd, 0, SEEK_SET);
  file->eof = 0;
  file->err = 0;
}

int fgetc(FILE* file) {
  if (file == NULL || file->eof) {
    return EOF;
  }
  unsigned char c;
  int n = read(file->fd, &c, 1);
  if (n <= 0) {
    if (n == 0) {
      file->eof = 1;
    } else {
      file->err = 1;
    }
    return EOF;
  }
  return (int)c;
}

int fputc(int c, FILE* file) {
  if (file->eof) {
    return EOF;
  }
  unsigned char ch = (unsigned char)c;
  int n = write(file->fd, &ch, 1);
  if (n <= 0) {
    file->err = 1;
    return EOF;
  }
  return (int)ch;
}

char* fgets(char* s, int n, FILE* file) {
  if (s == NULL || n <= 0 || file == NULL) {
    return NULL;
  }
  int i = 0;
  while (i < n - 1) {
    int c = fgetc(file);
    if (c == EOF) {
      if (i == 0) {
        return NULL;
      }
      break;
    }
    s[i++] = (char)c;
    if (c == '\n') {
      break;
    }
  }
  s[i] = '\0';
  return s;
}

int fputs(const char* s, FILE* file) {
  size_t len = strlen(s);
  int n = write(file->fd, s, len);
  return (n < 0) ? EOF : n;
}

int feof(FILE* file) {
  if (file == NULL) {
    return 0;
  }
  return file->eof;
}

int ferror(FILE* file) {
  if (file == NULL) {
    return 0;
  }
  return file->err;
}

int fflush(FILE* file) {
  (void)file;
  return 0;  // All writes are unbuffered; nothing to flush.
}

int remove(const char* path) {
  (void)path;
  return -1;  // Not implemented.
}

int rename(const char* oldpath, const char* newpath) {
  (void)oldpath;
  (void)newpath;
  return -1;  // Not implemented.
}

#endif /* __is_libc */

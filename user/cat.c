#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

static int number_lines;

static int cat_fd(int fd) {
  static char buf[4096];
  int line = 1;
  int at_line_start = 1;
  int n;
  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    if (!number_lines) {
      int written = 0;
      while (written < n) {
        int w = write(1, buf + written, (size_t)(n - written));
        if (w < 0) {
          return 1;
        }
        written += w;
      }
    } else {
      for (int i = 0; i < n; ++i) {
        if (at_line_start) {
          printf("%6d\t", line);
          ++line;
          at_line_start = 0;
        }
        char c = buf[i];
        if (write(1, &c, 1) < 0) {
          return 1;
        }
        if (c == '\n') {
          at_line_start = 1;
        }
      }
    }
  }
  return n < 0 ? 1 : 0;
}

int main(int argc, char* argv[]) {
  int opt;
  while ((opt = getopt(argc, argv, "n")) != -1) {
    switch (opt) {
      case 'n':
        number_lines = 1;
        break;
      default:
        printf("usage: cat [-n] [file]...\n");
        return 1;
    }
  }

  if (optind >= argc) {
    return cat_fd(0);
  }

  int ret = 0;
  for (int i = optind; i < argc; ++i) {
    int fd = open(argv[i], O_RDONLY);
    if (fd < 0) {
      printf("cat: %s: No such file or directory\n", argv[i]);
      ret = 1;
      continue;
    }
    if (cat_fd(fd) != 0) {
      printf("cat: %s: read error\n", argv[i]);
      ret = 1;
    }
    close(fd);
  }
  return ret;
}

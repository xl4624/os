#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("usage: touch <file>...\n");
    return 1;
  }

  int ret = 0;
  for (int i = 1; i < argc; ++i) {
    int fd = open(argv[i], O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
      printf("touch: %s: cannot create file\n", argv[i]);
      ret = 1;
      continue;
    }
    close(fd);
  }
  return ret;
}

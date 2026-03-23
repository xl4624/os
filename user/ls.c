#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  const char* path = "/";

  if (argc >= 2) {
    path = argv[1];
  } else {
    /* Default to CWD when available. */
    static char cwd_buf[128];
    if (getcwd(cwd_buf, sizeof(cwd_buf)) != 0) {
      path = cwd_buf;
    }
  }

  DIR* d = opendir(path);
  if (d == NULL) {
    printf("ls: %s: no such directory\n", path);
    return 1;
  }

  const struct dirent* e;
  while ((e = readdir(d)) != NULL) {
    if (e->d_type == DT_DIR) {
      printf("%s/\n", e->d_name);
    } else if (e->d_type == DT_CHR) {
      printf("%s  [dev]\n", e->d_name);
    } else {
      printf("%-24s  %u bytes\n", e->d_name, e->d_size);
    }
  }
  closedir(d);
  return 0;
}

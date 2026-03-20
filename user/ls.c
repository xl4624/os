#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* getdents is declared in dirent.h but the libc wrapper lives in unistd/ */
int getdents(const char* path, struct dirent* buf, unsigned int count);

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

  static struct dirent entries[64];
  int n = getdents(path, entries, 64);
  if (n < 0) {
    printf("ls: %s: no such directory\n", path);
    return 1;
  }

  for (int i = 0; i < n; ++i) {
    const struct dirent* e = &entries[i];
    if (e->d_type == DT_DIR) {
      printf("%s/\n", e->d_name);
    } else if (e->d_type == DT_CHR) {
      printf("%s  [dev]\n", e->d_name);
    } else {
      printf("%-24s  %u bytes\n", e->d_name, e->d_size);
    }
  }
  return 0;
}

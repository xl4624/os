#include <dirent.h>
#include <stdlib.h>

DIR* opendir(const char* path) {
  if (path == NULL) {
    return NULL;
  }

  DIR* dirp = (DIR*)malloc(sizeof(DIR));
  if (dirp == NULL) {
    return NULL;
  }

  int n = getdents(path, dirp->entries, DIR_BUF_SIZE);
  if (n < 0) {
    free(dirp);
    return NULL;
  }

  dirp->count = n;
  dirp->index = 0;
  return dirp;
}

struct dirent* readdir(DIR* dirp) {
  if (dirp == NULL || dirp->index >= dirp->count) {
    return NULL;
  }
  return &dirp->entries[dirp->index++];
}

int closedir(DIR* dirp) {
  if (dirp == NULL) {
    return -1;
  }
  free(dirp);
  return 0;
}

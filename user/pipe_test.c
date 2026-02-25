#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) {
  int fds[2];
  if (pipe(fds) < 0) {
    printf("pipe_test: pipe() failed\n");
    return 1;
  }

  int pid = fork();
  if (pid < 0) {
    printf("pipe_test: fork() failed\n");
    return 1;
  }

  if (pid == 0) {
    /* Child: close write end, read from pipe. */
    close(fds[1]);

    char buf[64];
    int n = read(fds[0], buf, sizeof(buf) - 1);
    if (n > 0) {
      buf[n] = '\0';
      printf("child read: \"%s\" (%d bytes)\n", buf, n);
    } else {
      printf("child: read returned %d\n", n);
    }
    close(fds[0]);
    return 0;
  }

  /* Parent: close read end, write to pipe. */
  close(fds[0]);

  const char* msg = "hello from parent";
  int len = (int)strlen(msg);
  write(fds[1], msg, (unsigned)len);
  close(fds[1]);

  int status = 0;
  int ret;
  while ((ret = waitpid(-1, &status)) == 0) {
  }

  printf("pipe_test: parent done (child status=%d)\n", status);
  return 0;
}

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define LINE_MAX 128

/*
 * Read one line from stdin into buf (up to max-1 chars plus null terminator).
 * Echoes each character and handles backspace. Blocks until newline.
 * Returns the number of characters in the line (not counting the terminator).
 */
static int readline(char* buf, int max) {
  int n = 0;
  while (1) {
    char c;
    if (read(0, &c, 1) != 1) {
      continue;
    }
    if (c == '\n' || c == '\r') {
      putchar('\n');
      buf[n] = '\0';
      return n;
    }
    if (c == '\b' || c == 127) {
      if (n > 0) {
        n--;
        putchar('\b');
        putchar(' ');
        putchar('\b');
      }
      continue;
    }
    if (c >= ' ' && n < max - 1) {
      buf[n++] = c;
      putchar(c);
    }
  }
}

int main(void) {
  char line[LINE_MAX];

  printf("mysh -- type a binary name (e.g. hello.elf) or \"exit\"\n");

  while (1) {
    printf("$ ");

    if (readline(line, LINE_MAX) == 0) {
      continue;
    }

    if (strcmp(line, "exit") == 0) {
      break;
    }

    int pid = fork();
    if (pid < 0) {
      printf("sh: fork failed\n");
      continue;
    }

    if (pid == 0) {
      /* Child: replace this process image with the named binary. */
      if (exec(line) < 0) {
        printf("sh: not found: %s\n", line);
        _exit(1);
      }
      /* exec does not return on success. */
      _exit(0);
    }

    /* Parent: wait for the child to finish. */
    int status = 0;
    int ret;
    while ((ret = waitpid(-1, &status)) == 0) {
      /* blocked, retry when rescheduled. */
    }
    if (ret > 0 && status != 0) {
      printf("sh: exited with code %d\n", status);
    }
  }

  return 0;
}

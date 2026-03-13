#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define LINE_MAX     256
#define PATH_MAX     128
#define MAX_ARGS     16
#define MAX_CMDS     8
#define MAX_VARS     16
#define VAR_NAME_MAX 32
#define VAR_VAL_MAX  64

// ===========================================================================
// Shell variable store
// ===========================================================================

static char var_names[MAX_VARS][VAR_NAME_MAX];
static char var_vals[MAX_VARS][VAR_VAL_MAX];
static int num_vars = 0;

static void set_var(const char* name, const char* val) {
  for (int i = 0; i < num_vars; i++) {
    if (strcmp(var_names[i], name) == 0) {
      strncpy(var_vals[i], val, VAR_VAL_MAX - 1);
      var_vals[i][VAR_VAL_MAX - 1] = '\0';
      return;
    }
  }
  if (num_vars < MAX_VARS) {
    strncpy(var_names[num_vars], name, VAR_NAME_MAX - 1);
    var_names[num_vars][VAR_NAME_MAX - 1] = '\0';
    strncpy(var_vals[num_vars], val, VAR_VAL_MAX - 1);
    var_vals[num_vars][VAR_VAL_MAX - 1] = '\0';
    num_vars++;
  }
}

static const char* get_var(const char* name) {
  for (int i = 0; i < num_vars; i++) {
    if (strcmp(var_names[i], name) == 0) {
      return var_vals[i];
    }
  }
  return 0;
}

// ===========================================================================
// Input reading
// ===========================================================================

/*
 * Read one line from stdin into buf (up to max-1 chars plus null terminator).
 * Echoes each character and handles backspace. Blocks until newline.
 * Returns the number of characters in the line (not counting the terminator).
 */
static int readline(char* buf, int max) {
  int n = 0;
  while (1) {
    char c;
    if (read(0, &c, 1) != 1) continue;
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

// ===========================================================================
// Variable expansion
// ===========================================================================

static int is_var_char(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_';
}

/* Expand $VAR references from src into dst (at most dsz bytes including NUL). */
static void expand_vars(const char* src, char* dst, int dsz) {
  int si = 0, di = 0;
  while (src[si] && di < dsz - 1) {
    if (src[si] != '$') {
      dst[di++] = src[si++];
      continue;
    }
    si++;  /* skip '$' */
    char vname[VAR_NAME_MAX];
    int vi = 0;
    while (is_var_char(src[si]) && vi < (int)sizeof(vname) - 1) {
      vname[vi++] = src[si++];
    }
    vname[vi] = '\0';
    const char* val = get_var(vname);
    if (val) {
      while (*val && di < dsz - 1) dst[di++] = *val++;
    }
  }
  dst[di] = '\0';
}

// ===========================================================================
// Command parsing
// ===========================================================================

/* A single command: argv[0] is the program name. */
typedef struct {
  char* argv[MAX_ARGS];
  int argc;
} Cmd;

/* A pipeline of one or more commands connected by pipes. */
typedef struct {
  Cmd cmds[MAX_CMDS];
  int ncmds;
} Pipeline;

/* Advance past leading spaces; trim trailing spaces in place. */
static char* trim(char* s) {
  while (*s == ' ') s++;
  int len = (int)strlen(s);
  while (len > 0 && s[len - 1] == ' ') s[--len] = '\0';
  return s;
}

/*
 * Parse a space-delimited command segment into cmd.
 * Modifies cmd_str in place (strtok_r writes NULs).
 */
static void parse_cmd(char* cmd_str, Cmd* cmd) {
  cmd->argc = 0;
  char* save;
  char* tok = strtok_r(cmd_str, " ", &save);
  while (tok && cmd->argc < MAX_ARGS - 1) {
    cmd->argv[cmd->argc++] = tok;
    tok = strtok_r(0, " ", &save);
  }
  cmd->argv[cmd->argc] = 0;
}

/*
 * Split line by '|' and parse each segment into pl.
 * Modifies line in place.
 */
static void parse_pipeline(char* line, Pipeline* pl) {
  pl->ncmds = 0;
  char* save;
  char* seg = strtok_r(line, "|", &save);
  while (seg && pl->ncmds < MAX_CMDS) {
    seg = trim(seg);
    parse_cmd(seg, &pl->cmds[pl->ncmds++]);
    seg = strtok_r(0, "|", &save);
  }
}

// ===========================================================================
// Builtins
// ===========================================================================

/* Returns 1 if word is a VAR=value assignment and handles it; 0 otherwise. */
static int try_assignment(const char* word) {
  /* Variable names must start with a letter or underscore. */
  if (!(word[0] >= 'a' && word[0] <= 'z') &&
      !(word[0] >= 'A' && word[0] <= 'Z') && word[0] != '_') {
    return 0;
  }
  const char* eq = strchr(word, '=');
  if (!eq) return 0;
  /* All characters before '=' must be valid name characters. */
  for (const char* p = word; p < eq; p++) {
    if (!is_var_char(*p)) return 0;
  }
  char name[VAR_NAME_MAX];
  int nlen = (int)(eq - word);
  if (nlen >= VAR_NAME_MAX) return 0;
  strncpy(name, word, (size_t)nlen);
  name[nlen] = '\0';
  set_var(name, eq + 1);
  return 1;
}

/*
 * Try to run cmd as a builtin. Returns 1 if handled (including error cases),
 * 0 if not a builtin.
 */
static int run_builtin(Cmd* cmd) {
  if (strcmp(cmd->argv[0], "help") == 0) {
    printf("Built-in commands:\n");
    printf("  exit          exit the shell\n");
    printf("  help          show this message\n");
    printf("  cd DIR        (not yet supported: requires chdir syscall)\n");
    printf("  NAME=VALUE    set a shell variable\n");
    printf("  $NAME         expand a shell variable\n");
    printf("Pipeline:  cmd1 | cmd2\n");
    return 1;
  }
  if (strcmp(cmd->argv[0], "exit") == 0) {
    _exit(0);
  }
  if (strcmp(cmd->argv[0], "cd") == 0) {
    printf("sh: cd: not supported (no chdir syscall)\n");
    return 1;
  }
  return 0;
}

// ===========================================================================
// Pipeline execution
// ===========================================================================

static void wait_child(int pid) {
  int status = 0;
  int ret;
  while ((ret = waitpid(pid, &status)) == 0) {
    /* blocked; retry when rescheduled */
  }
  if (ret > 0 && status != 0) {
    printf("sh: [%d] exited with status %d\n", pid, status);
  }
}

static void run_pipeline(Pipeline* pl) {
  if (pl->ncmds == 0) return;

  /* Single command: check for assignment or builtin before forking. */
  if (pl->ncmds == 1) {
    Cmd* cmd = &pl->cmds[0];
    if (cmd->argc == 0) return;
    if (try_assignment(cmd->argv[0])) return;
    if (run_builtin(cmd)) return;
  }

  /* Create (ncmds - 1) pipes connecting consecutive commands. */
  int pipes[MAX_CMDS - 1][2];
  int npipes = pl->ncmds - 1;
  for (int i = 0; i < npipes; i++) {
    if (pipe(pipes[i]) < 0) {
      printf("sh: pipe failed\n");
      for (int j = 0; j < i; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }
      return;
    }
  }

  /* Fork one child per command and wire up its stdin/stdout. */
  int pids[MAX_CMDS];
  for (int i = 0; i < pl->ncmds; i++) {
    Cmd* cmd = &pl->cmds[i];
    if (cmd->argc == 0) {
      pids[i] = -1;
      continue;
    }

    int pid = fork();
    if (pid < 0) {
      printf("sh: fork failed\n");
      pids[i] = -1;
      continue;
    }

    if (pid == 0) {
      /* Child: read stdin from the previous pipe. */
      if (i > 0) dup2(pipes[i - 1][0], 0);
      /* Child: write stdout into the next pipe. */
      if (i < npipes) dup2(pipes[i][1], 1);
      /* Close all pipe ends; the child only uses the dup2'd copies. */
      for (int j = 0; j < npipes; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }
      /* Build VFS path: /bin/<command> */
      char path[PATH_MAX];
      const char* name = cmd->argv[0];
      if (name[0] == '/') {
        /* Already an absolute path. */
        strncpy(path, name, PATH_MAX - 1);
        path[PATH_MAX - 1] = '\0';
      } else {
        /* Prepend /bin/ to bare command name. */
        strncpy(path, "/bin/", 6);
        strncpy(path + 5, name, PATH_MAX - 6);
        path[PATH_MAX - 1] = '\0';
      }
      if (exec(path) < 0) {
        printf("sh: not found: %s\n", name);
        _exit(1);
      }
      _exit(0);
    }

    pids[i] = pid;
  }

  /* Parent: close all pipe ends so children get EOF when peers exit. */
  for (int i = 0; i < npipes; i++) {
    close(pipes[i][0]);
    close(pipes[i][1]);
  }

  /* Wait for every child to finish. */
  for (int i = 0; i < pl->ncmds; i++) {
    if (pids[i] > 0) wait_child(pids[i]);
  }
}

// ===========================================================================
// Main
// ===========================================================================

int main(void) {
  char raw[LINE_MAX];
  char expanded[LINE_MAX];

  printf("mysh -- type a command (try \"help\")\n");

  while (1) {
    printf("$ ");

    if (readline(raw, LINE_MAX) == 0) continue;

    /* Expand $VAR references before parsing. */
    expand_vars(raw, expanded, LINE_MAX);

    Pipeline pl;
    parse_pipeline(expanded, &pl);
    run_pipeline(&pl);
  }

  return 0;
}

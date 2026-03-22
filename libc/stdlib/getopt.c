#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

char* optarg = NULL;
int optind = 1;
int opterr = 1;
int optopt = 0;

/* Position within the current argv element being scanned. */
static int optpos = 0;

int getopt(int argc, char* const argv[], const char* optstring) {
  optarg = NULL;

  if (optind >= argc) {
    return -1;
  }

  const char* arg = argv[optind];

  /* Skip non-option arguments and bare "-". */
  if (arg == NULL || arg[0] != '-' || arg[1] == '\0') {
    return -1;
  }

  /* "--" terminates option scanning. */
  if (arg[1] == '-' && arg[2] == '\0') {
    ++optind;
    return -1;
  }

  /* Advance past the '-' on first visit to this argv element. */
  if (optpos == 0) {
    optpos = 1;
  }

  int c = (unsigned char)arg[optpos];
  ++optpos;

  const char* match = strchr(optstring, c);
  if (match == NULL) {
    optopt = c;
    if (opterr != 0) {
      printf("%s: invalid option -- '%c'\n", argv[0], c);
    }
    /* Advance to next argv element if this one is exhausted. */
    if (arg[optpos] == '\0') {
      ++optind;
      optpos = 0;
    }
    return '?';
  }

  /* Check if this option takes an argument. */
  if (match[1] == ':') {
    if (arg[optpos] != '\0') {
      /* Argument is the rest of the current argv element. */
      optarg = (char*)&arg[optpos];
    } else if (match[2] != ':') {
      /* Required argument: take the next argv element. */
      ++optind;
      if (optind >= argc) {
        optopt = c;
        if (opterr != 0) {
          printf("%s: option requires an argument -- '%c'\n", argv[0], c);
        }
        optpos = 0;
        return (optstring[0] == ':') ? ':' : '?';
      }
      optarg = argv[optind];
    }
    /* For optional argument (::), optarg stays NULL if nothing follows. */
    ++optind;
    optpos = 0;
  } else {
    /* No argument expected. Advance if this element is exhausted. */
    if (arg[optpos] == '\0') {
      ++optind;
      optpos = 0;
    }
  }

  return c;
}

int getopt_long(int argc, char* const argv[], const char* optstring, const struct option* longopts,
                int* longindex) {
  optarg = NULL;

  if (optind >= argc) {
    return -1;
  }

  const char* arg = argv[optind];

  if (arg == NULL || arg[0] != '-') {
    return -1;
  }

  /* "--" terminates option scanning. */
  if (arg[1] == '-' && arg[2] == '\0') {
    ++optind;
    return -1;
  }

  /* Check for long option ("--foo" or "--foo=bar"). */
  if (arg[1] == '-') {
    const char* name = arg + 2;
    size_t name_len = 0;
    const char* eq = strchr(name, '=');
    if (eq != NULL) {
      name_len = (size_t)(eq - name);
    } else {
      name_len = strlen(name);
    }

    for (int i = 0; longopts[i].name != NULL; ++i) {
      if (strncmp(longopts[i].name, name, name_len) != 0 || longopts[i].name[name_len] != '\0') {
        continue;
      }

      /* Found a match. */
      if (longindex != NULL) {
        *longindex = i;
      }
      ++optind;

      if (longopts[i].has_arg == required_argument || longopts[i].has_arg == optional_argument) {
        if (eq != NULL) {
          optarg = (char*)(eq + 1);
        } else if (longopts[i].has_arg == required_argument) {
          if (optind >= argc) {
            optopt = longopts[i].val;
            if (opterr != 0) {
              printf("%s: option '--%s' requires an argument\n", argv[0], longopts[i].name);
            }
            return (optstring[0] == ':') ? ':' : '?';
          }
          optarg = argv[optind];
          ++optind;
        }
      }

      if (longopts[i].flag != NULL) {
        *longopts[i].flag = longopts[i].val;
        return 0;
      }
      return longopts[i].val;
    }

    /* No long option matched. */
    optopt = 0;
    if (opterr != 0) {
      printf("%s: unrecognized option '%s'\n", argv[0], arg);
    }
    ++optind;
    return '?';
  }

  /* Short option: delegate to getopt(). */
  return getopt(argc, argv, optstring);
}

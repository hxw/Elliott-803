// main.c

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "emulator.h"

// display usage message and exit
static void usage(const char *program, const char *format, ...) {

  if (NULL != format) {
    fprintf(stderr, "error: ");
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr, "usage: %s [options] [arguments]\n", program);
  fprintf(stderr, "       -h           this message\n");
  fprintf(stderr, "       -e FILE      execute a file of commands\n");
  fprintf(stderr, "       -i           interactive mode (after commands)\n");
  fprintf(stderr, "       -V           display program version\n");

  exit(EXIT_FAILURE);
}

// main program
int main(int argc, char *argv[]) {

  static const char *program = PROGRAM_STRING;
  static const char *version = VERSION_STRING;

  FILE *f = NULL;
  int ch = 0;
  bool interactive = false;
  while ((ch = getopt(argc, argv, "e:hiV")) != -1) {
    switch (ch) {
    case 'e':
      if (NULL != f) {
        usage(program, "only one -e option is permitted");
      }
      f = fopen(optarg, "r");
      if (NULL == f) {
        usage(program, "file: %s  error: %s\n", optarg, strerror(errno));
      }
      break;

    case 'i':
      interactive = true;
      break;

    case 'V':
      printf("%s version: %s\n", program, version);
      return 0;

    case 'h':
    case '?':
      usage(program, NULL);

    default:
      usage(program, "invalid option: %c", ch);
    }
  }
  argc -= optind;
  argv += optind;

#if 0
  printf("argc: %d\n", argc);
  for (int i = 0; i < argc; ++i) {
    printf("argv[%2d]: %s\n", i, argv[i]);
  }
  exit(99);
#endif

  int rc = emulator(program, version, f, interactive, argc, argv);
  if (NULL != f) {
    fclose(f);
    f = NULL;
  }
  return rc;
}

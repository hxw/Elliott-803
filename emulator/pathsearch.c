// pathsearch.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#include "pathsearch.h"

ps_error_t path_search(char *buffer,
                       size_t buffer_length,
                       const char *filename,
                       const wchar_t *filename_wc) {
  ps_error_t rc = PS_ok;

  char *path = NULL;
  char *p = getenv("E803_TAPE_DIR");
  if (NULL == p) {
#if defined(DEFAULT_TAPE_DIR)
    path = malloc(sizeof(DEFAULT_TAPE_DIR));
#else
    path = malloc(1);
#endif
    if (NULL == path) {
      rc = PS_malloc_failed;
      goto clean_up;
    }
    *path = '\0'; // just the current directory will be searched
#if defined(DEFAULT_TAPE_DIR)
    strlcat(path, DEFAULT_TAPE_DIR, sizeof(DEFAULT_TAPE_DIR));
#endif
  } else {
    size_t len = strlen(p) + 2; // colon prefix and trailing '\0'
    path = malloc(len);
    if (NULL == path) {
      rc = PS_malloc_failed;
      goto clean_up;
    }
    path[0] = ':'; // current directory will be searched
    path[1] = '\0';
    strlcat(path, p, len); // append rest of env
  }

  p = path;
  const char *q = NULL;
  while (NULL != (q = strsep(&p, ":"))) {
    const char *s = '\0' == *q ? "" : "/";
    size_t n =
      snprintf(buffer, buffer_length, "%s%s%s%ls", q, s, filename, filename_wc);
    if (n >= buffer_length - 1) {
      rc = PS_filename_too_long;
      goto clean_up;
    }

    if (0 == access(buffer, R_OK)) {
      rc = PS_ok;
      goto clean_up;
    }
  }

  rc = PS_file_not_found;

clean_up:
  if (NULL != path) {
    free(path);
  }
  return rc;
}

// pathsearch.h

#if !defined(PATHSEARCH_H)
#define PATHSEARCH_H

#include <stdint.h>
#include <wchar.h>

typedef enum {
  PS_ok,
  PS_malloc_failed,
  PS_filename_too_long,
  PS_file_not_found,
} ps_error_t;

ps_error_t path_search(char *buffer,
                       size_t buffer_length,
                       const char *filename,
                       const wchar_t *filename_wc);

#endif

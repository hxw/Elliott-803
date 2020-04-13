// read.c

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "io5.h"
#include "structs.h"

// return 8 bit binary value from input channel
ssize_t io5_file_read(io5_file_t *file, uint8_t *buffer, size_t length) {

  if (NULL == file || NULL == file->conv || NULL == buffer || length < 1 ||
      io_direction_read != file->direction) {
    return -1;
  }

  FILE *in = file->handle;
  if (NULL == in) {
    return -1;
  }

  ssize_t r = 0;
  while (length > 0) {
    ssize_t gn = io5_conv_get(file->conv, &buffer[r], length);
    r += gn;
    length -= gn;
    if (0 == gn) {
      if (0 == file->end) {
        size_t n = fread(file->buffer, 1, sizeof(file->buffer), in);
        if (n <= 0) {
          break;
        }
        file->start = 0;
        file->end = n;
      }
      size_t n = file->end - file->start;
      ssize_t np = io5_conv_put(file->conv, &file->buffer[file->start], n);
      if (np < 0) {
        return -1;
      }
      file->start += np;
      if (file->start >= file->end) {
        file->start = 0;
        file->end = 0;
      }
    }
  }
  return r;
}

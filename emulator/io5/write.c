// write.c

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "io5.h"
#include "structs.h"

// return 8 bit binary value from input channel
ssize_t io5_file_write(io5_file_t *file, const uint8_t *buffer, size_t length) {

  if (NULL == file || NULL == file->conv || NULL == buffer || length < 1 ||
      io_direction_write != file->direction) {
    return -1;
  }

  FILE *out = file->handle;
  if (NULL == out) {
    return -1;
  }

  ssize_t n = 0;
  while (length > 0) {

    size_t np = io5_conv_put(file->conv, &buffer[n], length);

    n += np;
    length -= np;

    if (0 == n) {
      break;
    }

    uint8_t temp[256];
    size_t ng = io5_conv_get(file->conv, temp, sizeof(temp));

    if (0 == ng) {
      break;
    }

    size_t nw = fwrite(temp, 1, ng, out);
    if (nw != ng) {
      return -1;
    }
  }
  return n;
}

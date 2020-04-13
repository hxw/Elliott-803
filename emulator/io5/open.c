// open.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io5.h"
#include "structs.h"

static io5_error_t internal_open(io5_file_t *file, const char *name,
                                 const char *open_mode,
                                 io_direction_t direction, io5_mode_t mode) {
  if (NULL == file) {
    return io5_error;
  }

  (void)io5_file_close(file);

  if (mode < io5_mode_hex5 || mode >= io5_mode_count) {
    return io5_error;
  }

  char *n = strdup(name);
  if (NULL == n) {
    return io5_error;
  }
  FILE *f = fopen(name, open_mode);
  if (NULL == f) {
    free(n);
    return io5_error;
  }
  file->name = n;
  file->handle = f;
  file->direction = direction;

  if (NULL != file->conv) {
    io5_conv_deallocate(file->conv);
    file->conv = NULL;
  }

  io5_conv_t *conv = NULL;
  if (io_direction_write == direction) {
    conv = io5_conv_allocate(io5_mode_binary, mode);
  } else {
    conv = io5_conv_allocate(mode, io5_mode_binary);
  }
  if (NULL == conv) {
    return io5_error;
  }
  file->conv = conv;
  memset(file->buffer, 0, sizeof(file->buffer));
  file->start = 0;
  file->end = 0;
  return io5_ok;
}

// create a new files an attach to the io instance for writing
// will close any existing attachment first
io5_error_t io5_file_create(io5_file_t *file, const char *name,
                            io5_mode_t mode) {
  return internal_open(file, name, "wx", io_direction_write, mode);
}

// ope a io for reading and attache to the io instance
// will close any existing attachment first
io5_error_t io5_file_open(io5_file_t *file, const char *name, io5_mode_t mode) {
  return internal_open(file, name, "r", io_direction_read, mode);
}

// close an io stream
io5_error_t io5_file_close(io5_file_t *file) {
  if (NULL == file) {
    return io5_error;
  }
  if (NULL != file->handle) {
    fclose(file->handle);
    file->handle = NULL;
  }
  if (NULL != file->name) {
    free(file->name);
    file->name = NULL;
  }
  if (NULL != file->conv) {
    io5_conv_deallocate(file->conv);
    file->conv = NULL;
  }
  memset(file, 0, sizeof(io5_file_t));

  return io5_ok;
}

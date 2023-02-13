// allocation.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io5.h"
#include "structs.h"

// allocate an io stream instance not connected to any file
// returns NULL if cannot allocate memory
io5_file_t *io5_file_allocate(void) {
  io5_file_t *f = malloc(sizeof(io5_file_t));
  if (NULL == f) {
    return NULL;
  }
  memset(f, 0, sizeof(io5_file_t));
  return f;
}

// deallocate all resources attached to the instance
io5_error_t io5_file_deallocate(io5_file_t *file) {
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

  memset(file, 0, sizeof(io5_file_t));
  free(file);

  return io5_ok;
}

// allocate an io stream instance not connected to any file
// returns NULL if cannot allocate memory
io5_conv_t *io5_conv_allocate(io5_mode_t from, io5_mode_t to) {
  io5_conv_t *e = malloc(sizeof(io5_conv_t));
  if (NULL == e) {
    return NULL;
  }
  memset(e, 0, sizeof(io5_conv_t));

  e->from = from;
  e->to = to;
  return e;
}

// deallocate all resources attached to the instance
io5_error_t io5_conv_deallocate(io5_conv_t *conv) {
  if (NULL == conv) {
    return io5_error;
  }

  memset(conv, 0, sizeof(io5_conv_t));
  free(conv);

  return io5_ok;
}

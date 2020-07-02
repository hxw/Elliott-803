// io5.h

#if !defined(IO5_H)
#define IO5_H

#include <stdint.h>

// i/o mode conversion
typedef enum {
  io5_mode_hex5, // default
  io5_mode_hex8,
  io5_mode_binary,
  io5_mode_elliott,

  io5_mode_count,   // number of items
  io5_mode_invalid, // to represent unset/error value
} io5_mode_t;

typedef enum {
  io5_ok = 0,
  io5_error = 1,
} io5_error_t;

typedef struct io5_file_struct io5_file_t;

// allocate an io stream instance not connected to any file
// returns NULL if cannot allocate memory
io5_file_t *io5_file_allocate(void);

// deallocate all resources attached to the instance
io5_error_t io5_file_deallocate(io5_file_t *file);

// create a new files an attach to the io instance for writing
// will close any existing attachment first
io5_error_t
io5_file_create(io5_file_t *file, const char *name, io5_mode_t mode);

// open a file for reading and attache to the io instance
// will close any existing attachment first
// in hex files with legible tape leader the two lines:
//    #skip
//    #endskip
// to allow the read function to ignore these characters
io5_error_t io5_file_open(io5_file_t *file, const char *name, io5_mode_t mode);

// close an io stream
io5_error_t io5_file_close(io5_file_t *file);

// read data from an io stream
// returns:
//   +N   number of characters read
//    0   end of file
//   -1   error
ssize_t io5_file_read(io5_file_t *file, uint8_t *buffer, size_t length);

// write data to an io stream
// returns:
//   +N   number of characters written
//    0   end of file
//   -1   error
ssize_t io5_file_write(io5_file_t *file, const uint8_t *buffer, size_t length);

// ------------------------------------------------------------

// converter
typedef struct io5_conv_struct io5_conv_t;

// allocate an io converter
// returns NULL if cannot allocate memory
io5_conv_t *io5_conv_allocate(io5_mode_t from, io5_mode_t to);

// deallocate all resources attached to the instance
io5_error_t io5_conv_deallocate(io5_conv_t *conv);

// send "characters" encoded as "from" to internal buffer
// returns:
//   +N   number of characters consumed (maybe zero)
size_t io5_conv_put(io5_conv_t *conv, const uint8_t *buffer, size_t length);

// read internal buffer and convert as "to" "characters"
// returns:
//   +N   number of characters returned (maybe zero)
size_t io5_conv_get(io5_conv_t *conv, uint8_t *buffer, size_t length);

#endif

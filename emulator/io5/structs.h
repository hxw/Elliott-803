// structs.h

#if !defined(STRUCTS_H)
#define STRUCTS_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <wchar.h>

#include "io5.h"

typedef enum {
  io_direction_inactive = 0,
  io_direction_read = 1,
  io_direction_write = 2,
} io_direction_t;

typedef enum {
  shift_null = 0,
  shift_figures = 1,
  shift_letters = 2,
} shift_t;

struct io5_file_struct {
  char *name;               // strdup of current file
  FILE *handle;             // NULL if closed
  io_direction_t direction; // whether read or write is allowed
  io5_conv_t *conv;         // converter
  uint8_t buffer[256];      // temp buffer
  size_t start;             // first occupied byte in buffer
  size_t end;               // first free byte in buffer
};

typedef enum {
  state_begin,
  state_hex,
  state_eol,
  state_hash,
  state_skip,
  state_skip_hash,
  state_utf8_5, // must be consecutive, descending
  state_utf8_4, // must be consecutive, descending
  state_utf8_3, // must be consecutive, descending
  state_utf8_2, // must be consecutive, descending
  state_utf8_1, // must be consecutive, descending
} state_t;

struct io5_conv_struct {
  io5_mode_t from;      // conversion mode
  io5_mode_t to;        // conversion mode
  shift_t shift_from;   // to keep current shift for in
  shift_t shift_to;     // to keep current shift for out
  bool skip;            // to handle skipping legible leaders
  shift_t shift;        // to keep current shift for output
  uint8_t buffer[1024]; // fixed size read/write buffer
  size_t put;           // next free byte in buffer
  size_t get;           // next readable byte (if == put then empty)
  state_t from_state;   // state machine for decoding input stream
  uint8_t from_byte[3]; // "XX\0"  decode hex values
  wint_t from_wchar;    // assemble a wide char from UTF-8 bytes
};

#endif

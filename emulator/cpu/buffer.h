// buffer.h

#if !defined(BUFFER_H)
#define BUFFER_H 1

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// I/O channel buffers

enum {
  buffer_capacity = 1023,
};

typedef struct {
  uint8_t data[buffer_capacity + 1];
  size_t read_position;
  size_t write_position;
} buffer_t;

// clear buffer storage and set as empty
static inline void buffer_clear(buffer_t *buffer) {
  memset(buffer, 0, sizeof(buffer_t));
}

// returns:
//   true  if byte was saved
//   false if buffer is full and byte was rejected
static inline bool buffer_put(buffer_t *buffer, uint8_t b) {
  buffer->data[buffer->write_position] = b;
  size_t new_write = buffer->write_position + 1;
  if (new_write >= sizeof(buffer->data)) {
    new_write = 0;
  }
  if (new_write == buffer->read_position) {
    return false;
  }
  buffer->write_position = new_write;
  return true;
}

// returns:
//   true  if byte was retrieved
//   false if buffer is empty
static inline bool buffer_get(buffer_t *buffer, uint8_t *b_ptr) {
  if (buffer->read_position == buffer->write_position) {
    return false;
  }
  *b_ptr = buffer->data[buffer->read_position];
  size_t new_read = buffer->read_position + 1;
  if (new_read >= sizeof(buffer->data)) {
    new_read = 0;
  }
  buffer->read_position = new_read;
  return true;
}

#endif

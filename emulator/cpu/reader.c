// reader.c

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "processor.h"
#include "pts.h"

// return 8 bit binary value from input channel
bool pts_reader(processor_t *proc, int unit, uint8_t *c) {
  if (NULL == proc || unit < 1 || unit > reader_units) {
    return false;
  }
  buffer_t *io = &proc->reader[unit - 1];

  return buffer_get(io, c);
}

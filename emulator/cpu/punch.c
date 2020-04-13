// punch.c

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "processor.h"
#include "pts.h"

// output 5..8 bit value
// returns busy status
bool pts_punch(processor_t *proc, int unit, uint8_t c) {
  if (NULL == proc || unit < 1 || unit > punch_units) {
    return false;
  }
  buffer_t *io = &proc->punch[unit - 1];
  return buffer_put(io, c);
}

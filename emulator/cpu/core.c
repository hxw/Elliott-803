// core.c

#include "core.h"
#include "processor.h"

static const uint64_t T1[4] = {
  ELLIOTT(026, 4, 0, 006, 0), // 0
  ELLIOTT(022, 4, 1, 016, 3), // 1
  ELLIOTT(055, 5, 0, 071, 0), // 2
  ELLIOTT(043, 1, 0, 040, 2), // 3
};

// read data from core; zero for boot loader
int64_t core_read(processor_t *proc, int address) {
  address &= address_bits;
  if (address < 4) {
    return 0;
  }
  return proc->core_store[address];
}

// read boot loader or core
int64_t core_read_program(processor_t *proc, int address) {
  address &= address_bits;
  if (address < 4) {
    return T1[address];
  }
  return proc->core_store[address];
}

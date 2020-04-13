// core.h

#if !defined(CORE_H)
#define CORE_H 1

#include "processor.h"

int64_t core_read(processor_t *proc, int address);
int64_t core_read_program(processor_t *proc, int address);

#endif

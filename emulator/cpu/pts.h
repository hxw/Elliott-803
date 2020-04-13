// pts.h

#if !defined(PTS_H)
#define PTS_H 1

#include "processor.h"

bool pts_punch(processor_t *proc, int unit, uint8_t c);
bool pts_reader(processor_t *proc, int unit, uint8_t *c);

#endif

// emulator.h

#if !defined(EMULATOR_H)
#define EMULATOR_H

#include <stdbool.h>
#include <stdio.h>

int emulator(const char *name, const char *version, FILE *f, bool interactive);

#endif

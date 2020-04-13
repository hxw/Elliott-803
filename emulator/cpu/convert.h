// pts.h

#if !defined(PTS_H)
#define PTS_H 1

#include <stdint.h>

// convert machine code string to a word
// returns:
//   a word fully left shifted to be correct for store to core
//   on error -1LL
int64_t from_machine_code(const char *s, size_t length);

// convert word to machine code, hex octal and signed decimal
// must free the returned string after use
char *to_machine_code(const char *prefix, int64_t word);

#endif

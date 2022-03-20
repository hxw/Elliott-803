// convert.c

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "convert.h"
#include "processor.h"

// convert machine code string to a word
// returns:
//   a word fully left shifted to be correct for store to core
//   on error -1LL
int64_t from_machine_code(const char *s, size_t length) {

  int64_t w = 0;
  int state = 0;
  int address = 0;
  int shift = 64;
  bool negative = false;
  bool decimal = false;

  for (size_t i = 0; i < length; ++i) {
    int c = s[i];
    if (' ' == c || '\t' == c) {
      continue;
    }
    switch (state) {
    case 0: // op 1
      if ('-' == c) {
        state = 6;
        decimal = true;
        negative = true;
        break;
      }
      if ('+' == c) {
        state = 6;
        decimal = true;
        break;
      }
      __attribute__((fallthrough));
    case 1: // op 1
    case 3: // op 2
    case 4: // op 2
      if (c >= '0' && c <= '7') {
        ++state;
        shift -= 3;
        w |= (int64_t)(c - '0') << shift;
      } else {
        return -1LL;
      }
      break;
    case 2: // address 1
    case 5: // address 2
      if (2 == state && ('/' == c || ':' == c)) {
        shift -= 13;
        w |= (int64_t)(address & 8191) << shift;
        address = 0;
        shift -= 1;
        if ('/' == c) {
          w |= 1LL << shift;
        }
        ++state;
      } else if (c >= '0' && c <= '9') {
        address = 10 * address + c - '0';
      } else {
        return -1LL;
      }
      break;
    case 6: // ±decimal
      if (c >= '0' && c <= '9') {
        w = 10 * w + c - '0';
      } else {
        return -1LL;
      }
      break;
    }
  }
  if (decimal) {
    if (negative) {
      w = -w;
    }
    w <<= second_address_shift;
  } else {
    // final address
    shift -= 13;
    w |= (int64_t)(address & 8191) << shift;
  }
  return w;
}

// convert word to machine code, hex octal and signed decimal
// must free the returned string after use
char *to_machine_code(const char *prefix, int64_t word) {

  int op1 = op_bits & (word >> first_op_shift);
  int addr1 = address_bits & (word >> first_address_shift);
  const char *b = 0 == (word & b_mod_bit) ? ":" : "/";
  int op2 = op_bits & (word >> second_op_shift);
  int addr2 = address_bits & (word >> second_address_shift);

  uint64_t value = (word >> word_shift) & lsb_thirty_nine_bits;

  int64_t value_signed = value;
  if (0 != (value_signed & (1LL << 38))) {
    value_signed |= lsb_sign_extend_bits;
  }

  char *p = NULL;

  // this allocates memory for the returned string
  asprintf(&p,
           "%s%02o %4d %s %02o %4d   X%010" PRIx64 "  8%013" PRIo64
           "  %+13" PRId64,
           prefix,
           op1,
           addr1,
           b,
           op2,
           addr2,
           value,
           value,
           value_signed);

  return p;
}

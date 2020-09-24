// fpu.c

#include <stdio.h>

#include "fpu.h"
#include "processor.h"

// clang-format: off
// 30 bit 2's complement signed mantissa:
// range ½ ≤ m < 1 or -1 ≤ m < -½

//          S 29 bit positive                9 bit exp
//  zero  = 0 00000000000000000000000000000  000000000
//  -1    = 1 00000000000000000000000000000  100000000
//   1    = 0 10000000000000000000000000000  100000001
//  30    = 0 11110000000000000000000000000  100000101
//-0.0875 = 1 01001111111111111111111111111  011111101

// 9 bit signed exponent as a positive integer 0 ≤ (e+256) ≤ 511
int64_t fpu_standardise(int64_t a) {
  if (0 == a) {
    return a;
  }
  int64_t exponent = exponent_offset + 38;
  int64_t mantissa = a;
  for (;;) {
    int64_t t = mantissa & top_two_bits;
    if (!(0 == t || top_two_bits == t)) {
      break;
    }
    mantissa <<= 1;
    --exponent;
  }

  if (0 != (mantissa & underflow_bits)) {
    mantissa |= epsilon_bit;
  }
  return (mantissa & mantissa_bits) |
         ((exponent << exponent_shift) & exponent_bits);
}

// addition
int64_t fpu_add(bool *overflow, int64_t a, int64_t b) {
  if (0 == a) {
    return b;
  } else if (0 == b) {
    return a;
  }

  int64_t ma = a & mantissa_bits;
  int64_t ea = (a & exponent_bits) >> exponent_shift;
  int64_t mb = b & mantissa_bits;
  int64_t eb = (b & exponent_bits) >> exponent_shift;

  // printf("fpu_add:\n");
  // printf("ma: %016lx  ea: %ld\n", ma, ea);
  // printf("mb: %016lx  eb: %ld\n", mb, eb);

  // ensure ea is the larger value
  if (ea < eb) {
    int64_t t = ma;
    ma = mb;
    mb = t;
    t = ea;
    ea = eb;
    eb = t;
  }

  // printf("ma: %016lx  ea: %ld\n", ma, ea);
  // printf("mb: %016lx  eb: %ld\n", mb, eb);

  // printf("ea-eb: %ld\n", ea - eb);

  for (int64_t i = ea - eb; i > 0; --i) {
    int64_t s = mb & sign_bit;
    mb = (mb >> 1) | s;
  }

  // printf("mb: %016lx\n", mb);

  int64_t s = ma & sign_bit;
  ma = (ma >> 1) | s;
  // printf("a>: %016lx\n", ma);

  s = mb & sign_bit;
  ma += (mb >> 1) | s;
  // printf("a+: %016lx\n", ma);
  ++ea;

  if (0 == ma) {
    return 0;
  }

  ma = fpu_standardise(ma);
  // printf("qs: %013llo  x%016lx\n", B39(ma), ma);
  // printf("qs: %013lo  x%016lx\n", ma, ma);

  eb = (ma & exponent_bits) >> exponent_shift;
  ea += eb - 38 - exponent_offset;

  s = ma & sign_bit;
  if (ea < 0) {
    return 0; // underflow
  } else if (ea >= 2 * exponent_offset) {
    *overflow = true;
    return (0 == s) ? fpu_pos_overflow : fpu_neg_overflow;
  }
  return (ma & mantissa_bits) | ((ea << exponent_shift) & exponent_bits);
}

// negate
int64_t fpu_neg(int64_t a) {
  if (0 == a) {
    return 0;
  }
  int64_t ma = a & mantissa_bits;
  int64_t ea = (a & exponent_bits) >> exponent_shift;

  int64_t s = ma & sign_bit;
  ma = -((ma >> 1) | s);
  ++ea;

  for (;;) {
    int64_t t = ma & top_two_bits;
    if (!(0 == t || top_two_bits == t)) {
      break;
    }
    ma <<= 1;
    --ea;
    // printf("ma: %016lx  ea: %ld\n", ma, ea);
  }

  if (ea < 0) {
    return 0; // underflow
  } else if (ea >= 2 * exponent_offset) {
    //*overflow = true;
    return (0 != s) ? fpu_pos_overflow : fpu_neg_overflow;
  }

  return (ma & mantissa_bits) | ((ea << exponent_shift) & exponent_bits);
}

// multiply
int64_t fpu_mpy(bool *overflow, int64_t a, int64_t b) {
  if (0 == a || 0 == b) {
    return 0;
  }

  int64_t sa = a & sign_bit;
  int64_t sb = b & sign_bit;

  int64_t ma = (a & mantissa_bits) >> mantissa_shift;
  int64_t ea = (a & exponent_bits) >> exponent_shift;
  int64_t mb = (b & mantissa_bits) >> mantissa_shift;
  int64_t eb = (b & exponent_bits) >> exponent_shift;

  if (0 != sa) {
    ma |= mantissa_sign_extend_bits;
  }
  if (0 != sb) {
    mb |= mantissa_sign_extend_bits;
  }

  eb += ea - 2 * exponent_offset;

  ma = fpu_standardise(ma * mb);

  ea = (ma & exponent_bits) >> exponent_shift;

  // printf("before: ea: %ld\n", ea);
  // unclear why 33 *****FIX THIS*****
  ea += eb - 33;

  // printf("after:  ea: %ld\n", ea);
  if (ea < 0) {
    return 0; // underflow
  } else if (ea >= 2 * exponent_offset) {
    *overflow = true;
    return (sa == sb) ? fpu_pos_overflow : fpu_neg_overflow;
  }

  return (ma & mantissa_bits) | ((ea << exponent_shift) & exponent_bits);
}

// division
int64_t fpu_div(bool *overflow, int64_t a, int64_t b) {
  if (0 == a) {
    return 0;
  } else if (0 == b) {
    *overflow = true;
    return fpu_pos_overflow; // need to set overflow
  }
  // printf("division\n");

  int64_t ma = a & mantissa_bits;
  int64_t ea = (a & exponent_bits) >> exponent_shift;
  int64_t mb = b & mantissa_bits;
  int64_t eb = (b & exponent_bits) >> exponent_shift;

  // printf("ea: %03lo\n", ea);
  // printf("eb: %03lo\n", eb);

  // right justify
  mb >>= mantissa_shift;
  ma /= mb;
  // printf("qq: %013llo  x%016lx\n", B39(ma), ma);

  ma = fpu_standardise(ma);
  // printf("qs: %013llo  x%016lx\n", B39(ma), ma);

  eb = ea - eb;
  // printf("ed: %+ld\n", eb);

  ea = (ma & exponent_bits) >> exponent_shift;
  // printf("ex: %03lo\n", ea);
  // printf("ez: %03lo\n", ea - 34);

  ea += eb - exponent_size;
  ma &= mantissa_bits;

  // printf("ma: %013llo\n", B39(ma));
  // printf("ea: %03lo\n", ea);

  if (ea < 0) {
    return 0; // underflow
  } else if (ea >= 2 * exponent_offset) {
    int64_t sa = a & sign_bit;
    int64_t sb = b & sign_bit;
    *overflow = true;
    return (sa == sb) ? fpu_pos_overflow : fpu_neg_overflow;
  }

  return (ma & mantissa_bits) | ((ea << exponent_shift) & exponent_bits);
}

// fpu.h

#if !defined(FPU_H)
#define FPU_H 1

#include <stdbool.h>
#include <stdint.h>

#include "constants.h"

// Floating point numbers are represented in a 39 bit word

static const int64_t mantissa_size = 30LL;
static const int64_t exponent_size = 9LL;

static const int64_t mantissa_bits = 0xfffffffc00000000LL;
static const int64_t mantissa_shift = word_shift + exponent_size;
static const int64_t mantissa_sign_extend_bits = 0xffffffffc0000000LL;

static const int64_t epsilon_bit = 0x0000000400000000LL;
static const int64_t underflow_bits = 0x0000000377777777LL;

static const int64_t exponent_bits = 0x00000003fe000000LL;
static const int64_t exponent_shift = word_shift;
static const int64_t exponent_offset = 256;

// maximums + and -
static const int64_t fpu_pos_overflow = 0x7ffffffffe000000LL;
static const int64_t fpu_neg_overflow = 0x80000003fe000000LL;

// functions
int64_t fpu_standardise(int64_t a);
int64_t fpu_neg(int64_t a);
int64_t fpu_add(bool *overflow, int64_t a, int64_t b);
int64_t fpu_mpy(bool *overflow, int64_t a, int64_t b);
int64_t fpu_div(bool *overflow, int64_t a, int64_t b);

#endif

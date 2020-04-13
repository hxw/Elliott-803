// constants.h

#if !defined(CONSTANTS_H)
#define CONSTANTS_H 1

#include <stdint.h>

// structure of a 39 bit value in 64 bit machine
// sign(1) | value(38) | zero bits(25)

static const int64_t half_bit = 0x0000000001000000LL;
static const int64_t one_bit = 0x0000000002000000LL;

static const int64_t sign_bit = 0x8000000000000000LL;
static const int64_t top_two_bits = 0xc000000000000000LL;
static const int64_t ar_msb = 0x4000000000000000LL;

// exclude sign bit
static const int64_t thirty_eight_bits = 0x7ffffffffe000000LL;

// include sign bit
static const int64_t thirty_nine_bits = 0xfffffffffe000000LL;

static const int64_t lsb_thirty_nine_bits = 0x0000007fffffffffLL;
static const int64_t lsb_sign_extend_bits = 0xffffff8000000000LL;

//    6          5           4          3          2         1         0
// 321098 7654321098765 4 321098 7654321098765 4321098765432109876543210
// FFFFFF AAAAAAAAAAAAA B FFFFFF AAAAAAAAAAAAA 0000000000000000000000000
static const int64_t first_op_shift = 58;
static const int64_t first_address_shift = 45;
static const int64_t second_op_shift = 38;
static const int64_t second_address_shift = 25;

// to align left/right justified 39 bit values
static const int64_t word_shift = 25;

static const int64_t op_a_bits = 0xffffe00000000000LL;
static const int64_t op_b_bits = 0x00000ffffe000000LL;

static const int64_t b_mod_bit = 1LL << 44;

static const int64_t op_bits = 077;
static const int64_t address_bits = 8191;

static const int64_t memory_size = 8192;

// I/O limits
// reader 1/2, punch 1/2
// teleprinter 3 (i/o)
static const int reader_units = 3;
static const int punch_units = 3;

#define ELLIOTT(OP1, A1, B, OP2, A2)                                           \
  ((OP1##LL << first_op_shift) | (A1##LL << first_address_shift) |             \
   (B ? b_mod_bit : 0) | (OP2##LL << second_op_shift) |                        \
   (A2##LL << second_address_shift))

#if !defined(SizeOfArray)
#define SizeOfArray(a) (sizeof(a) / sizeof((a)[0]))
#endif

#endif

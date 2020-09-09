// alu.c

#include "alu.h"
#include "constants.h"

// execute the base operation from groups 0..3
int64_t alu_add(bool *overflow, int op, int64_t acc, int64_t a, int64_t n) {

  switch (op & 7) {
  default:
  case 0: // no-op
    return a;

  case 1: { // negate: -a
    if (0 == a) {
      return 0;
    }
    int64_t ap = -a;
    if (a == ap) {
      *overflow = true;
    }
    return ap;
  }

  case 2: { // increment: n+1
    int64_t np = n + one_bit;
    // detect overflow + to -
    if ((0 == (n & sign_bit)) && (0 != (np & sign_bit))) {
      *overflow = true;
    }
    return np;
  }

  case 3: // collate: a&n
    return acc & n;

  case 4: { // add: a+n
    int64_t a1 = acc;
    int overflow_flags = 0 == (a1 & sign_bit) ? 0 : 4;
    int64_t a2 = n;
    overflow_flags |= 0 == (a2 & sign_bit) ? 0 : 2;

    int64_t sum = a1 + a2;
    overflow_flags |= 0 == (sum & sign_bit) ? 0 : 1;

    if (overflow_flags == 1 || overflow_flags == 6) {
      *overflow = true;
    }
    return sum;
  }

  case 5: { // subtract: a-n
    int64_t a1 = acc;
    int overflow_flags = 0 == (a1 & sign_bit) ? 0 : 4;
    int64_t a2 = n;
    overflow_flags |= 0 == (a2 & sign_bit) ? 0 : 2;

    int64_t sum = a1 - a2;
    overflow_flags |= 0 == (sum & sign_bit) ? 0 : 1;

    if (overflow_flags == 3 || overflow_flags == 4) {
      *overflow = true;
    }
    return sum;
  }

  case 6: // clear: zero
    return 0;

  case 7: { // negate and add: n-a
    int64_t a1 = n;
    int overflow_flags = 0 == (a1 & sign_bit) ? 0 : 4;
    int64_t a2 = acc;
    overflow_flags |= 0 == (a2 & sign_bit) ? 0 : 2;

    int64_t sum = a1 - a2;
    overflow_flags |= 0 == (sum & sign_bit) ? 0 : 1;

    if (overflow_flags == 3 || overflow_flags == 4) {
      *overflow = true;
    }
    return sum;
  }
  }
}

// double length multiplication
void alu_multiply(int64_t *acc, int64_t *ar, int64_t md1, int64_t mr1) {

  // printf("multiply:\n");
  // printf("md : %016lx\n", md1);
  // printf("mr : %016lx\n", mr1);

  uint64_t md = (uint64_t)(md1);
  uint64_t mr = (uint64_t)(mr1);

  bool negative = false;
  if (md1 < 0) {
    md = -md;
    negative = !negative;
  }
  if (mr1 < 0) {
    mr = -mr;
    negative = !negative;
  }
  // printf("mdu: %016lx\n", md);
  // printf("mru: %016lx\n", mr);

  // split into 32 bit and 7 bit parts
  uint64_t mdh = (md >> 32) & 0xffffffffULL;
  uint64_t mdl = (md >> (32 - 7)) & 0x7fULL;
  uint64_t mrh = (mr >> 32) & 0xffffffffULL;
  uint64_t mrl = (mr >> (32 - 7)) & 0x7fULL;

  // printf("mdh: %016lx\n", mdh);
  // printf("mdl: %016lx\n", mdl);
  // printf("mrh: %016lx\n", mrh);
  // printf("mrl: %016lx\n", mrl);

  uint64_t low = mdl * mrl; // 14 bits
  // printf("low: %016lx\n", low);
  uint64_t mid = (mdh * mrl + mdl * mrh) << 7; // 39 bits + 7 zeros
  // printf("mid: %016lx\n", mid);

  mid += low;                // low 38 bits
  uint64_t high = mdh * mrh; // 64 bits  implied 14 zeros below LSB
  // printf("hi : %016lx\n", high);

  mid += (high << 14) & 0x3fffffc000ULL;
  // printf("mi+: %016lx\n", mid);

  uint64_t al = (mid << 25) & thirty_eight_bits;

  high += (mid & 0x3fc000000000ULL) >> 14; // propagate carry from mid
  // printf("hi+: %016lx\n", high);
  uint64_t ah = (high << 1) & thirty_nine_bits;

  if (negative) {
    al = (~al) & thirty_eight_bits;
    ah = (~ah) & thirty_nine_bits;
    al += one_bit;
    if (0 != (al & sign_bit)) {
      ah += one_bit;
    }
    al &= thirty_eight_bits;
  }

  *acc = ah;
  *ar = al;
}

// 128-bit / 64-bit unsigned divide
// derived from:
//   https://codereview.stackexchange.com/questions/67962/mostly-portable-128-by-64-bit-division
static uint64_t internal_divide(uint64_t dividend_high, uint64_t dividend_low,
                                uint64_t divisor) {

  uint64_t quotient = dividend_low << 1;
  uint64_t remainder = dividend_high;

  uint64_t carry = dividend_low >> 63;
  uint64_t temp_carry = 0;

  for (int i = 0; i < 64; i++) {
    temp_carry = remainder >> 63;
    remainder <<= 1;
    remainder |= carry;
    carry = temp_carry;

    if (carry == 0) {
      if (remainder >= divisor) {
        carry = 1;
      } else {
        temp_carry = quotient >> 63;
        quotient <<= 1;
        quotient |= carry;
        carry = temp_carry;
        continue;
      }
    }

    remainder -= divisor;
    remainder -= (1 - carry);
    carry = 1;
    temp_carry = quotient >> 63;
    quotient <<= 1;
    quotient |= carry;
    carry = temp_carry;
  }

  return quotient;
}

// double length division
int64_t alu_divide(bool *overflow, int64_t acc, int64_t ar, int64_t divisor) {

  if (0 == divisor) {
    *overflow = true;
    return acc;
  }
  bool negate = false;
  if (acc < 0) {
    negate = !negate;
    acc = ~acc & thirty_nine_bits;
    ar = (~ar & thirty_eight_bits) + one_bit;
    if (0 != (ar & sign_bit)) {
      acc += one_bit;
      ar &= thirty_eight_bits;
    }
  }
  if (divisor < 0) {
    negate = !negate;
    divisor = -divisor;
  }

  uint64_t dh = (uint64_t)(acc) >> 1;
  dh |= ar >> (word_shift + 38 - (64 - 39) + 1);

  uint64_t dl = (uint64_t)(ar) << (word_shift + 39 - 38 - 1);

  // printf("acc ar:   %016lx %016lx\n"
  //        "dh  dl:   %016lx %016lx\n"
  //        "divisor:  %016lx %016lx\n",
  //        acc >> word_shift, ar >> word_shift, dh, dl, divisor,
  //        divisor >> word_shift);

  uint64_t q = internal_divide(dh, dl, divisor);
  if (negate) {
    q = -q;
  }
  q &= thirty_nine_bits;

  // printf("quotient: %016lx\n", q);

  return q;
}

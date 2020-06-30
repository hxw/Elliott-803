// fpu_test.c

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "fpu.h"

static void test_1_arg(const char *title,
                       int64_t (*f)(int64_t a),
                       int64_t a,
                       int64_t result,
                       bool overflow) {
  int64_t actual = f(a);

  if (actual != result) {
    printf("%-16s: actual:   %013llo (%s)\n"
           "%-16s  expected: %013llo (%s)\n",
           title,
           (actual >> exponent_shift) & 0x7fffffffffULL,
           "_",
           "",
           (result >> exponent_shift) & 0x7fffffffffULL,
           overflow ? "V" : "_");
    exit(1);
  }
}

static void test_2_args(const char *title,
                        int64_t (*f)(bool *overflow, int64_t a, int64_t b),
                        int64_t a,
                        int64_t b,
                        int64_t result,
                        bool overflow) {
  bool actual_overflow = false;
  int64_t actual = f(&actual_overflow, a, b);

  if (actual != result || actual_overflow != overflow) {
    printf("%-16s: actual:   %013llo (%s)\n"
           "%-16s  expected: %013llo (%s)\n",
           title,
           (actual >> exponent_shift) & 0x7fffffffffULL,
           actual_overflow ? "V" : "_",
           "",
           (result >> exponent_shift) & 0x7fffffffffULL,
           overflow ? "V" : "_");
    exit(1);
  }
}

int main(int argc, char *argv[]) {
  //  zero  = 0 00000000000000000000000000000  000000000
  //  -1    = 1 00000000000000000000000000000  100000000
  //   1    = 0 10000000000000000000000000000  100000001
  //  30    = 0 11110000000000000000000000000  100000101
  //-0.0875 = 1 01001111111111111111111111111  011111101

#define FLOAT803(x) ((x) << exponent_shift)

  const int64_t zero = 0;
  const int64_t neg_0_5 = FLOAT803(04000000000377LL);
  const int64_t neg_1 = FLOAT803(04000000000400LL);
  const int64_t neg_2 = FLOAT803(04000000000401LL);

  const int64_t pos_0_25 = FLOAT803(02000000000377LL);
  const int64_t pos_0_5 = FLOAT803(02000000000400LL);
  const int64_t pos_1 = FLOAT803(02000000000401LL);
  const int64_t pos_2 = FLOAT803(02000000000402LL);
  const int64_t pos_3 = FLOAT803(03000000000402LL);
  const int64_t pos_4 = FLOAT803(02000000000403LL);

  const int64_t pos_10 = FLOAT803(02400000000404LL);
  const int64_t pos_10_5 = FLOAT803(02500000000404LL);

  const int64_t pos_10_5_div_10 = FLOAT803(02063146314401LL);
  const int64_t pos_10_div_10_5 = FLOAT803(03636363636400LL);

  const int64_t pos_100 = FLOAT803(03100000000407LL);
  const int64_t pos_105 = FLOAT803(03220000000407LL);

  const int64_t pos_1000 = FLOAT803(03720000000412LL);
  const int64_t pos_10000 = FLOAT803(02342000000416LL);

  const int64_t pos_max = FLOAT803(03777777777777LL);
  const int64_t pos_max_div_2 = FLOAT803(03777777777776LL);

  const int64_t neg_max = FLOAT803(04000000000777LL);

#if 0
2150: 20    0 : 00  257   X: 2000000101  2000000000401   137438953729  1.0
2151: 24    0 : 00  260   X: 2800000104  2400000000404   171798692100  10.0
2152: 31    0 : 00  263   X: 3200000107  3100000000407   214748365063  100.0
2153: 37 2048 : 00  266   X: 3e8000010a  3720000000412   268435456266
2154: 23 4352 : 00  270   X: 271000010e  2342000000416   167772160270
2155: 30 3392 : 00  273   X: 30d4000111  3032400000421   209715200273
2156: 36 4240 : 00  276   X: 3d09000114  3641100000424   262144000276
2157: 23  602 : 00  280   X: 2625a00118  2304550000430   163840000280
2158: 27 6896 / 00  283   X: 2faf08011b  2765702000433   204800000283
2159: 35 6572 / 20  286   X: 3b9aca011e  3563262400436   256000000286
2160: 22 5131 / 62  290   X: 2540be4122  2250057440442   160000000290
2161: 27 2318 / 56 4389   X: 2e90edd125  2722073350445   200000000293
2162: 35  850 / 12 1320   X: 3a35294528  3506512242450   250000000296
#endif

  // standardise

  test_1_arg("stand      0", fpu_standardise, 0, zero, false);
  test_1_arg(
    "stand     -2", fpu_standardise, -2ULL << word_shift, neg_2, false);
  test_1_arg(
    "stand     -1", fpu_standardise, -1ULL << word_shift, neg_1, false);
  test_1_arg("stand      1", fpu_standardise, 1LL << word_shift, pos_1, false);
  test_1_arg("stand      2", fpu_standardise, 2LL << word_shift, pos_2, false);
  test_1_arg(
    "stand     10", fpu_standardise, 10LL << word_shift, pos_10, false);
  test_1_arg(
    "stand    100", fpu_standardise, 100LL << word_shift, pos_100, false);
  test_1_arg(
    "stand  1,000", fpu_standardise, 1000LL << word_shift, pos_1000, false);
  test_1_arg(
    "stand 10,000", fpu_standardise, 10000LL << word_shift, pos_10000, false);

  // addition

  test_2_args("add  +1 + -1", fpu_add, pos_1, neg_1, zero, false);
  test_2_args("add  -1 + +1", fpu_add, neg_1, pos_1, zero, false);
  test_2_args("add  +1 + +1", fpu_add, pos_1, pos_1, pos_2, false);
  test_2_args("add  -1 + -1", fpu_add, neg_1, neg_1, neg_2, false);
  test_2_args("add  -1 +  0", fpu_add, neg_1, zero, neg_1, false);
  test_2_args("add   0 + -1", fpu_add, zero, neg_1, neg_1, false);
  test_2_args("add  +1 +  0", fpu_add, pos_1, zero, pos_1, false);
  test_2_args("add   0 + +1", fpu_add, zero, pos_1, pos_1, false);

  test_2_args("add  +1 + +2", fpu_add, pos_1, pos_2, pos_3, false);
  test_2_args("add  +2 + +1", fpu_add, pos_2, pos_1, pos_3, false);

  test_2_args("add -max-max", fpu_add, neg_max, neg_max, neg_max, true);
  test_2_args("add +max+max", fpu_add, pos_max, pos_max, pos_max, true);

  test_1_arg("negate     0", fpu_neg, zero, zero, false);
  test_1_arg("negate    +1", fpu_neg, pos_1, neg_1, false);
  test_1_arg("negate    -1", fpu_neg, neg_1, pos_1, false);
  test_1_arg("negate    +2", fpu_neg, pos_2, neg_2, false);

  // multiplication

  test_2_args("mpy 1 * 0    ", fpu_mpy, pos_1, zero, zero, false);
  test_2_args("mpy 1 * 1    ", fpu_mpy, pos_1, pos_1, pos_1, false);
  test_2_args("mpy 1 * 2    ", fpu_mpy, pos_1, pos_2, pos_2, false);
  test_2_args("mpy 2 * 2    ", fpu_mpy, pos_2, pos_2, pos_4, false);
  test_2_args("mpy -2 * -2  ", fpu_mpy, neg_2, neg_2, pos_4, false);
  test_2_args("mpy 2 * 0.5  ", fpu_mpy, pos_2, pos_0_5, pos_1, false);
  test_2_args("mpy -1 * 0.5 ", fpu_mpy, neg_1, pos_0_5, neg_0_5, false);
  test_2_args("mpy 0.5 * 0.5", fpu_mpy, pos_0_5, pos_0_5, pos_0_25, false);
  test_2_args("mpy 10 * 10.5", fpu_mpy, pos_10, pos_10_5, pos_105, false);
  test_2_args("mpy 10 * 10  ", fpu_mpy, pos_10, pos_10, pos_100, false);
  test_2_args("mpy 10 * max ", fpu_mpy, pos_10, pos_max, pos_max, true);

  // division

  test_2_args("div  max / max", fpu_div, pos_max, pos_max, pos_1, false);
  test_2_args("div  max / 2  ", fpu_div, pos_max, pos_2, pos_max_div_2, false);
  test_2_args("div  max / 0.5", fpu_div, pos_max, pos_0_5, pos_max, true);

  test_2_args("div   0 / +1", fpu_div, zero, pos_1, zero, false);
  test_2_args("div   0 / -1", fpu_div, zero, neg_1, zero, false);

  test_2_args("div  +1 /  0", fpu_div, pos_1, zero, pos_max, true);
  test_2_args("div  -1 /  0", fpu_div, neg_1, zero, pos_max, true);

  test_2_args("div  +1 / +1", fpu_div, pos_1, pos_1, pos_1, false);
  test_2_args("div  +1 / +2", fpu_div, pos_1, pos_2, pos_0_5, false);
  test_2_args("div  2 / 0.5", fpu_div, pos_2, pos_0_5, pos_4, false);
  test_2_args("div -1 / 0.5", fpu_div, neg_1, pos_0_5, neg_2, false);

  test_2_args("div  10 / 10", fpu_div, pos_10, pos_10, pos_1, false);
  test_2_args(
    "div  10.5/10", fpu_div, pos_10_5, pos_10, pos_10_5_div_10, false);
  test_2_args(
    "div  10/10.5", fpu_div, pos_10, pos_10_5, pos_10_div_10_5, false);

  return 0;
}

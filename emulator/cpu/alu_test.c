// alu_test.c

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "alu.h"
#include "constants.h"

static void test_add_op(const char *title, int op, int64_t acc, int64_t a,
                        int64_t n, int64_t result, bool overflow) {

  bool actual_overflow = false;
  int64_t actual = alu_add(&actual_overflow, op, acc, a, n);

  if (actual != result || actual_overflow != overflow) {
    printf("%-16s: actual:   %013lo (%s)\n"
           "%-16s  expected: %013lo (%s)\n",
           title, (actual >> word_shift) & lsb_thirty_nine_bits,
           actual_overflow ? "v" : "_", "",
           (result >> word_shift) & lsb_thirty_nine_bits, overflow ? "V" : "_");
    exit(1);
  }
}

static void test_mpy(const char *title, int64_t a, int64_t b, int64_t acc,
                     int64_t ar) {
  int64_t actual_acc;
  int64_t actual_ar;
  alu_multiply(&actual_acc, &actual_ar, a, b);

  if (actual_acc != acc || actual_ar != ar) {
    printf("%-16s: actual:   %013lo (%013lo)\n"
           "%-16s  expected: %013lo (%013lo)\n",
           title, (actual_acc >> word_shift) & lsb_thirty_nine_bits,
           (actual_ar >> word_shift) & lsb_thirty_nine_bits, "",
           (acc >> word_shift) & lsb_thirty_nine_bits,
           (ar >> word_shift) & lsb_thirty_nine_bits);
    exit(1);
  }
}

static void test_div(const char *title, int64_t dh, int64_t dl, int64_t divisor,
                     int64_t expected_q) {

  bool overflow = false;
  int64_t actual_q = alu_divide(&overflow, dh, dl, divisor);
  //  actual_q <<= word_shift;

  if (actual_q != expected_q) {

    printf("%-16s: actual:   %013lo\n"
           "%-16s  expected: %013lo\n",
           title, (actual_q >> word_shift) & lsb_thirty_nine_bits, "",
           (expected_q >> word_shift) & lsb_thirty_nine_bits);
    exit(1);
  }
}

int main(int argc, char *argv[]) {

#define INT803(x) ((x) << word_shift)

  const int64_t zero = 0;

  const int64_t neg_1 = INT803(-1ULL);
  const int64_t neg_2 = INT803(-2ULL);

  const int64_t pos_1 = INT803(1LL);
  const int64_t pos_2 = INT803(2LL);
  const int64_t pos_3 = INT803(3LL);
  const int64_t pos_4 = INT803(4LL);
  const int64_t pos_5 = INT803(5LL);

  const int64_t pos_10 = INT803(10LL);

  const int64_t pos_max = INT803(0x3fffffffffLL);
  const int64_t pos_max_m1 = INT803(0x3ffffffffeLL);
  const int64_t pos_max_d2 = INT803(0x1fffffffffLL);
  const int64_t pos_highest = INT803(0x2000000000LL);

  const int64_t neg_max = INT803(0x4000000000LL);

  test_add_op("0: 1 1 1", 0, pos_1, pos_1, pos_1, pos_1, false);
  test_add_op("1: 1 1 1", 1, pos_1, pos_1, pos_1, neg_1, false);
  test_add_op("2: 1 1 1", 2, pos_1, pos_1, pos_1, pos_2, false);

  test_add_op("3: 3 1 1", 3, pos_3, pos_1, pos_1, pos_1, false);
  test_add_op("3: 3 1 4", 3, pos_3, pos_1, pos_4, zero, false);

  test_add_op("4: 2 0 2", 4, pos_2, zero, pos_2, pos_4, false);
  test_add_op("4: 2 0 2", 4, pos_5, zero, pos_5, pos_10, false);
  test_add_op("5: 3 0 1", 5, pos_3, zero, pos_1, pos_2, false);
  test_add_op("6: 1 1 1", 6, pos_1, pos_1, pos_1, zero, false);
  test_add_op("7: 3 0 1", 7, pos_3, zero, pos_1, neg_2, false);

  test_add_op("4: M 0 1", 4, pos_max, zero, pos_1, neg_max, true);
  test_add_op("5: M 0 1", 5, neg_max, zero, pos_1, pos_max, true);
  test_add_op("7: -1 0 M", 5, neg_1, zero, pos_max, neg_max, false);

  test_mpy("1 * 0", pos_1, zero, zero, zero);
  test_mpy("0 * 1", zero, pos_1, zero, zero);

  test_mpy("1 * 1", pos_1, pos_1, zero, pos_1);
  test_mpy("2 * 2", pos_2, pos_2, zero, pos_4);

  test_mpy("M * M", pos_max, pos_max, pos_max_m1, pos_1);
  test_mpy("-M * -M", neg_max, neg_max, neg_max, zero);

  test_div("1 / 1", zero, pos_1, pos_1, pos_1);
  test_div("2 / 1", zero, pos_2, pos_1, pos_2);
  test_div("2 / 2", zero, pos_2, pos_2, pos_1);
  test_div("10 / 2", zero, pos_10, pos_2, pos_5);
  test_div("max / max", zero, pos_max, pos_max, pos_1);

  test_div("max/2 / max", pos_max_d2, pos_highest, pos_max, pos_highest);
  test_div("max/2 / max", pos_max_d2, pos_max, pos_max, pos_highest);

  test_div("maxH / max", pos_max, zero, pos_max, sign_bit);
  test_div("maxHL / max", pos_max, pos_max, pos_max, sign_bit | pos_1);

  test_div("-1 / -1", neg_1, pos_max, neg_1, pos_1);
  test_div("-2 / -1", neg_1, pos_max_m1, neg_1, pos_2);
  test_div("-2 / -2", neg_1, pos_max_m1, neg_2, pos_1);

  test_div("1 / -1", zero, pos_1, neg_1, neg_1);
  test_div("2 / -1", zero, pos_2, neg_1, neg_2);
  test_div("2 / -2", zero, pos_2, neg_2, neg_1);

  test_div("-1 / 1", neg_1, pos_max, pos_1, neg_1);
  test_div("-2 / 1", neg_1, pos_max_m1, pos_1, neg_2);
  test_div("-2 / 2", neg_1, pos_max_m1, pos_2, neg_1);

  test_div("S / S", sign_bit, zero, sign_bit, sign_bit);

  return 0;
}

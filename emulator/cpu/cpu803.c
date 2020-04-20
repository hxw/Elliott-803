// cpu803.c

#include <stdio.h>

#include "alu.h"
#include "core.h"
#include "cpu803.h"
#include "fpu.h"
#include "processor.h"
#include "pts.h"

// instruction decoding and execution
static void cpu(processor_t *proc, int op, int address) {

  int64_t n = core_read(proc, address);
  int next_pc = proc->program_counter + 1;

  switch ((op >> 3) & 7) {

  case 0:
    // clang-format: off
    // Op  Operation                              a'        n'
    // 00  Do nothing                             a         n
    // 01  Negate                                 -a        n
    // 02  Replace & count                        n + 1     n
    // 03  Collate                                a & n     n
    // 04  Add                                    a + n     n
    // 05  Subtract                               a - n     n
    // 06  Clear                                  zero      n
    // 07  Negate & add                           n - a     n
    // clang-format: on
    proc->accumulator =
        alu_add(&proc->overflow, op, proc->accumulator, proc->accumulator, n);
    break;

  case 1:
    // clang-format: off
    // Op  Operation                              a'        n'
    // 10  Exchange                               n         a
    // 11  Exchange and negate                    -n        a
    // 12  Exchange and count                     n + 1     a
    // 13  Write and collate                      a & n     a
    // 14  Write and add                          a + n     a
    // 15  Write and subtract                     a - n     a
    // 16  Write and clear                        zero      a
    // 17  Write, negate and add                  n - a     a
    // clang-format: on
    proc->core_store[address] = proc->accumulator;
    proc->accumulator = alu_add(&proc->overflow, op, proc->accumulator, n, n);
    break;

  case 2:
    // clang-format: off
    // Op  Operation                              a'        n'
    // 20  Write                                  a         a
    // 21  Write negatively                       a         -a
    // 22  Count in store                         a         n + 1
    // 23  Collate in store                       a         a & n
    // 24  Add into store                         a         a + n
    // 25  Negate store and add                   a         a - n
    // 26  Clear store                            a         zero
    // 27  Subtract from store                    a         n - a
    // clang-format: on
    proc->core_store[address] =
        alu_add(&proc->overflow, op, proc->accumulator, proc->accumulator, n);
    break;

  case 3:
    // clang-format: off
    // Op  Operation                              a'        n'
    // 30  Replace                                n         n
    // 31  Replace and negate store               n         -n
    // 32  Replace and count in store             n         n + 1
    // 33  Replace and collate in store           n         a & n
    // 34  Replace and add to store               n         a + n
    // 35  Replace, negate store and add          n         a - n
    // 36  Replace and clear store                n         zero
    // 37  Replace and subtract from store        n         n - a
    // clang-format: on
    proc->core_store[address] =
        alu_add(&proc->overflow, op, proc->accumulator, n, n);
    proc->accumulator = n;
    break;

  case 4:
    // clang-format: off
    // 40  Transfer to 1st instruction unconditionally
    // 41  Transfer to 1st instruction if a is negative
    // 42  Transfer to 1st instruction if a is zero
    // 43  Transfer to 1st instruction if overflow set, and clear it
    // 44  Transfer to 2nd instruction unconditionally
    // 45  Transfer to 2nd instruction if a is negative
    // 46  Transfer to 2nd instruction if a is zero
    // 47  Transfer to 2nd instruction if overflow set, and clear it
    // clang-format: on
    switch (op & 7) {
    case 0:
      next_pc = address << 1;
      break;
    case 1:
      if (proc->accumulator < 0) {
        next_pc = address << 1;
      }
      break;
    case 2:
      if (0 == proc->accumulator) {
        next_pc = address << 1;
      }
      break;
    case 3:
      if (proc->overflow) {
        next_pc = address << 1;
        proc->overflow = false;
      }
      break;
    case 4:
      next_pc = (address << 1) | 1;
      break;
    case 5:
      if (proc->accumulator < 0) {
        next_pc = (address << 1) | 1;
      }
      break;
    case 6:
      if (0 == proc->accumulator) {
        next_pc = (address << 1) | 1;
      }
      break;
    case 7:
      if (proc->overflow) {
        next_pc = (address << 1) | 1;
        proc->overflow = false;
      }
      break;
    }
    break;

  case 5:
    // clang-format: off
    // 50  Arithmetic right shift a/ar N times
    // 51  Logical right shift a N times, clear ar (do not retain sign)
    // 52  Multiply a by n, result to a/ar
    // 53  Multiply a by n, single length rounded result to a, clear ar
    // 54  Arithmetic left shift a/ar N times
    // 55  Logical left shift a N times, clear ar
    // 56  Divide a/ar by n, single length quotient to a, clear ar
    // 57  Copy ar to a, set sign bit zero, do NOT clear the ar
    // clang-format: on
    switch (op & 7) {
    case 0: {
      for (int i = 0; i < address; ++i) {
        proc->accumulator >>= 1;
        proc->auxiliary_register >>= 1;
        if (0 != (proc->accumulator & half_bit)) {
          proc->auxiliary_register |= ar_msb;
        }
        proc->accumulator &= thirty_nine_bits;
        proc->auxiliary_register &= thirty_eight_bits;
      }
      break;
    }
    case 1:
      for (int i = 0; i < address; ++i) {
        proc->accumulator >>= 1;
        proc->accumulator &= thirty_eight_bits; // exclude sign
      }
      proc->auxiliary_register = 0;
      break;
    case 2:
      alu_multiply(&proc->accumulator, &proc->auxiliary_register,
                   proc->accumulator, n);
      break;
    case 3: {
      int64_t ah = 0;
      int64_t al = 0;
      alu_multiply(&ah, &al, proc->accumulator, n);

      if (thirty_nine_bits == ah) {
        proc->accumulator = al | sign_bit;
      } else if (0 == ah) {
        proc->accumulator = al;
      } else {
        proc->overflow = true;
      }
      proc->auxiliary_register = 0;
      break;
    }
    case 4: {
      bool negative = proc->accumulator < 0;
      for (int i = 0; i < address; ++i) {
        proc->auxiliary_register <<= 1;
        if (0 != (proc->auxiliary_register & sign_bit)) {
          proc->accumulator |= half_bit;
        }
        proc->accumulator <<= 1;
        if ((proc->accumulator < 0) != negative) {
          proc->overflow = true;
        }
      }
      proc->auxiliary_register &= thirty_eight_bits;
      break;
    }
    case 5: {
      bool negative = proc->accumulator < 0;
      for (int i = 0; i < address; ++i) {
        proc->accumulator <<= 1;
        if ((proc->accumulator < 0) != negative) {
          proc->overflow = true;
        }
      }
      proc->auxiliary_register = 0;
      break;
    }
    case 6:
      proc->accumulator = alu_divide(&proc->overflow, proc->accumulator,
                                     proc->auxiliary_register, n);
      proc->auxiliary_register = 0;
      break;
    case 7:
      proc->accumulator = proc->auxiliary_register;
      break;
    }

    break;

  case 6: // group 6 instructions clear the auxiliary register.
    proc->auxiliary_register = 0;

    // Floating point numbers are represented in a 39 bit word

    // const int64_t mantissa_bits = 0xfffffffc00000000LL;
    // const int64_t exponent_bits = 0x00000003fe000000LL;
    // const int64_t exponent_shift = word_shift;
    // const int64_t exponent_offset = 256;

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

    // Zero is always represented by all 39 bits zero

    // Note that the test for zero and test for negative jump instructions are
    // equally valid for floating point

    // Op  Operation                              a'        n'
    // 60  Add                                    a + n     n
    // 61  Subtract                               a - n     n
    // 62  Negate and Add                         n - a     n
    // 63  Multiply                               a * n     n
    // 64  Divide                                 a / n     n
    // 65  N = 4096: integer in the accumulator to floating point
    // 65  N < 4096: Fast left (end round) shift N mod 64 places
    // 66  (Spare)                                a         n
    // 67  (Spare)                                a         n
    // clang-format: on
    switch (op & 7) {
    case 0:
      proc->accumulator = fpu_add(&proc->overflow, proc->accumulator, n);
      break;
    case 1:
      proc->accumulator =
          fpu_add(&proc->overflow, proc->accumulator, fpu_neg(n));
      break;
    case 2:
      proc->accumulator =
          fpu_add(&proc->overflow, fpu_neg(proc->accumulator), n);
      break;
    case 3:
      proc->accumulator = fpu_mpy(&proc->overflow, proc->accumulator, n);
      break;
    case 4:
      proc->accumulator = fpu_div(&proc->overflow, proc->accumulator, n);
      break;
    case 5:
      if (address < 4096) {
        for (int i = 0; i < address; ++i) {
          if (0 != (proc->accumulator & sign_bit)) {
            proc->accumulator |= half_bit;
          }
          proc->accumulator <<= 1;
        }
      } else {
        proc->accumulator = fpu_standardise(proc->accumulator);
      }
      break;
    case 6:
      printf("66 not implemented\n");
      proc->mode = exec_mode_stop;
      break;
    case 7:
      printf("67 not implemented\n");
      proc->mode = exec_mode_stop;
      break;
    }
    break;
  case 7:
    // clang-format: off
    // 70  Read the keyboard number generator to the accumulator
    // 71  Read one char from the tape reader "or" it into A (ls 5..8 bits)
    //     0 = channel 1,  2048 = channel 2,  4096 = tty input
    // 72  Output to plotter
    // 73  Write the address of this instruction to location N
    // 74  Punch tape / teleprinter character N
    //     0 = channel 1,  2048 = channel 2,  4096 = tty output
    // 75  Film handler
    // 76  Film handler
    // 77  Block transfer
    // clang-format: on
    switch (op & 7) {
    case 0:
      proc->accumulator = proc->word_generator;
      break;
    case 1: {
      int unit = 1;
      busy_t reader_busy = busy_reader_1;
      if (0 != (address & 2048)) {
        n = 2;
        reader_busy = busy_reader_2;
      }
      if (0 != (address & 4096)) {
        n = 3;
        reader_busy = busy_reader_3;
      }
      uint8_t c = 0;
      if (!pts_reader(proc, unit, &c)) {
        proc->io_busy = reader_busy;
        return; // do not update program counter
      }
      proc->accumulator |= (int64_t)(c) << word_shift;
      break;
    }
    case 2:
      printf("72 not implemented\n");
      proc->mode = exec_mode_stop;
      break;
    case 3:
      // align integer part of program counter to second address
      // position of memory word
      proc->core_store[address] =
          ((int64_t)(proc->program_counter) << (second_address_shift - 1)) &
          thirty_nine_bits;
      break;
    case 4: {
      int unit = 1;
      busy_t punch_busy = busy_punch_1;
      if (0 != (address & 2048)) {
        unit = 2;
        punch_busy = busy_punch_2;
      }
      if (0 != (address & 4096)) {
        unit = 3;
        punch_busy = busy_punch_3;
      }
      if (!pts_punch(proc, unit, address)) {
        proc->io_busy = punch_busy;
        return; // do not update program counter
      }
      break;
    }
    case 5:
      printf("75 not implemented\n");
      proc->mode = exec_mode_stop;
      break;
    case 6:
      printf("76 not implemented\n");
      proc->mode = exec_mode_stop;
      break;
    case 7:
      printf("77 not implemented\n");
      proc->mode = exec_mode_stop;
      break;
    }
    break;
  }

  // update with incremented or destination of jump
  if (proc->program_counter == next_pc) {
    int op1 = op & 073;
    if (040 == op1 || 041 == op1 || 042 == op1) {
      proc->mode = exec_mode_stop;
    }
  }
  proc->program_counter = next_pc;
}

void cpu803_execute(processor_t *proc) {

  int64_t word = core_read_program(proc, proc->program_counter >> 1);

  if (0 == (1 & proc->program_counter)) {
    // first instruction
    int op = (word >> first_op_shift) & op_bits;
    int address = (word >> first_address_shift) & address_bits;
    cpu(proc, op, address);

  } else {

    if (0 != (b_mod_bit & word)) {
      int address = (word >> first_address_shift) & address_bits;
      int64_t modifier = core_read(proc, address);
      word += modifier;
    }
    // second instruction
    int op = (word >> second_op_shift) & op_bits;
    int address = (word >> second_address_shift) & address_bits;
    cpu(proc, op, address);
  }
}

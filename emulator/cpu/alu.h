// alu.h

#if !defined(ALU_H)
#define ALU_H 1

#include <stdbool.h>
#include <stdint.h>

int64_t alu_add(bool *overflow, int op, int64_t acc, int64_t a, int64_t n);
void alu_multiply(int64_t *acc, int64_t *ar, int64_t md1, int64_t mr1);
int64_t alu_divide(bool *overflow, int64_t dividend_high, int64_t dividend_low,
                   int64_t divisor);

#endif

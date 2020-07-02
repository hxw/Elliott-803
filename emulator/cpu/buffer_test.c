// buffer_test.c

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffer.h"

int main(int argc, char *argv[]) {

  buffer_t buffer;
  buffer_clear(&buffer);

  const uint8_t values[4] = {1, 13, 29, 88};

  uint8_t b;

  if (buffer_get(&buffer, &b)) {
    printf("unexpected value: %d  in empty buffer\n", b);
    return 1;
  }

  for (size_t i = 0; i < sizeof(values); ++i) {
    uint8_t p = values[i];
    if (!buffer_put(&buffer, p)) {
      printf("failed to store[%zu]: %d  in buffer\n", i, p);
      return 1;
    }
  }

  for (size_t i = 0; i < sizeof(values); ++i) {
    uint8_t p = values[i];
    if (!buffer_get(&buffer, &b)) {
      printf("missing value[%zu]: %d\n", i, p);
      return 1;
    }
    if (p != b) {
      printf("value[%zu]: %d != actual: %d\n", i, p, b);
      return 1;
    }
  }

  // check empty
  if (buffer_get(&buffer, &b)) {
    printf("unexpected value: %d  in empty buffer\n", b);
    return 1;
  }

  // fill and count
  for (int i = 0;; ++i) {
    if (!buffer_put(&buffer, 99)) {
      if (buffer_capacity == i) {
        break;
      }
      printf("failed to store[%d] in buffer\n", i);
      return 1;
    }
  }

  // empty and count
  for (int i = 0;; ++i) {
    if (!buffer_get(&buffer, &b)) {
      if (buffer_capacity == i) {
        break;
      }
      printf("failed to get[%d] from buffer\n", i);
      return 1;
    }
  }
}

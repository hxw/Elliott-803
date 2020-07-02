// conv_test.c

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "check.h"
#include "io5.h"
#include "structs.h"

static int do_test(void) {

  const uint8_t binary_data[] = {
    0x00, 0x00, 0x1c, 0x1d, 0x1e, // NUL NUL SP CR LF

    0x1f, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, // letters
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e,

    0x1b, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, // figures
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e,

    0x00, 0x00,
  };
  const uint8_t utf8_data[] = "\0\0 \r\n"                       // control
                              "abcdefghijklmnopqrstuvwxyz \r\n" // letters
                              "12*4<=78',+:-.>0()3?56/@9→ \r\n" // figures
                              "\0"; // also extra '\0' after end
  const uint8_t hex_data[] = "00\n00\n1c\n1d\n1e\n"
                             "1f\n01\n02\n03\n04\n05\n06\n07\n08\n09\n0a\n0b\n"
                             "0c\n0d\n0e\n0f\n10\n11\n12\n13\n14\n15\n16\n17\n"
                             "18\n19\n1a\n1c\n1d\n1e\n"
                             "1b\n01\n02\n03\n04\n05\n06\n07\n08\n09\n0a\n0b\n"
                             "0c\n0d\n0e\n0f\n10\n11\n12\n13\n14\n15\n16\n17\n"
                             "18\n19\n1a\n1c\n1d\n1e\n00\n00\n";

  int rc = 0;
  uint8_t actual[1024];

  // test binary encoding - effectively null
  // just to test internal ring buffer

  io5_conv_t *conv = io5_conv_allocate(io5_mode_binary, io5_mode_binary);
  if (NULL == conv) {
    printf("failed to allocate a conv\n");
    return 1;
  }

  size_t n = io5_conv_put(conv, binary_data, sizeof(binary_data));
  if (sizeof(binary_data) != n) {
    printf("error: consumed: %zu of: %zu\n", n, sizeof(binary_data));
    return 1;
  }

  memset(actual, 0, sizeof(actual));
  size_t actual_size = io5_conv_get(conv, actual, sizeof(actual));

  rc = check_data(actual, actual_size, binary_data, sizeof(binary_data));
  if (0 != rc) {
    printf("binary to binary failed\n");
    return rc;
  }

  if (io5_ok != io5_conv_deallocate(conv)) {
    printf("failed to deallocate conv\n");
    return 1;
  }

  // test binary to hex5 encoding

  conv = io5_conv_allocate(io5_mode_binary, io5_mode_hex5);
  if (NULL == conv) {
    printf("failed to allocate a conv\n");
    return 1;
  }

  n = io5_conv_put(conv, binary_data, sizeof(binary_data));
  if (sizeof(binary_data) != n) {
    printf("error: consumed: %zu of: %zu\n", n, sizeof(binary_data));
    return 1;
  }

  memset(actual, 0, sizeof(actual));
  actual_size = io5_conv_get(conv, actual, sizeof(actual));

  rc = check_data(actual,
                  actual_size,
                  hex_data,
                  sizeof(hex_data) - 1); // exclude trailing NUL
  if (0 != rc) {
    printf("binary to hex failed\n");
    return rc;
  }

  if (io5_ok != io5_conv_deallocate(conv)) {
    printf("failed to deallocate conv\n");
    return 1;
  }

  // test binary to Elliott encoding

  conv = io5_conv_allocate(io5_mode_binary, io5_mode_elliott);

  if (NULL == conv) {
    printf("failed to allocate a conv\n");
    return 1;
  }

  n = io5_conv_put(conv, binary_data, sizeof(binary_data));
  if (sizeof(binary_data) != n) {
    printf("error: consumed: %zu of: %zu\n", n, sizeof(binary_data));
    return 1;
  }

  memset(actual, 0, sizeof(actual));
  actual_size = io5_conv_get(conv, actual, sizeof(actual));

  rc = check_data(actual, actual_size, utf8_data, sizeof(utf8_data));
  if (0 != rc) {
    printf("binary to elliott failed\n");
    return rc;
  }

  if (io5_ok != io5_conv_deallocate(conv)) {
    printf("failed to deallocate conv\n");
    return 1;
  }

  // test hex5 to binary encoding

  conv = io5_conv_allocate(io5_mode_hex5, io5_mode_binary);
  if (NULL == conv) {
    printf("failed to allocate a conv\n");
    return 1;
  }

  n = io5_conv_put(conv,
                   hex_data,
                   sizeof(hex_data) - 1); // exclude trailing NUL
  if (sizeof(hex_data) - 1 != n) {
    printf("error: consumed: %zu of: %zu\n", n, sizeof(hex_data) - 1);
    return 1;
  }

  memset(actual, 0, sizeof(actual));
  actual_size = io5_conv_get(conv, actual, sizeof(actual));

  rc = check_data(actual, actual_size, binary_data, sizeof(binary_data));
  if (0 != rc) {
    printf("hex to binary failed\n");
    return rc;
  }

  if (io5_ok != io5_conv_deallocate(conv)) {
    printf("failed to deallocate conv\n");
    return 1;
  }

  // test Elliott to binary encoding

  conv = io5_conv_allocate(io5_mode_elliott, io5_mode_binary);
  if (NULL == conv) {
    printf("failed to allocate a conv\n");
    return 1;
  }

  n = io5_conv_put(conv, utf8_data, sizeof(utf8_data));
  if (sizeof(utf8_data) != n) {
    printf("error: consumed: %zu of: %zu\n", n, sizeof(utf8_data));
    return 1;
  }

  memset(actual, 0, sizeof(actual));
  actual_size = io5_conv_get(conv, actual, sizeof(actual));

  rc = check_data(actual, actual_size, binary_data, sizeof(binary_data));
  if (0 != rc) {
    printf("elliott to binary failed\n");
    return rc;
  }

  if (io5_ok != io5_conv_deallocate(conv)) {
    printf("failed to deallocate conv\n");
    return 1;
  }

  // test Elliott to hex encoding

  conv = io5_conv_allocate(io5_mode_elliott, io5_mode_hex5);
  if (NULL == conv) {
    printf("failed to allocate a conv\n");
    return 1;
  }

  n = io5_conv_put(conv, utf8_data, sizeof(utf8_data));
  if (sizeof(utf8_data) != n) {
    printf("error: consumed: %zu of: %zu\n", n, sizeof(utf8_data));
    return 1;
  }

  memset(actual, 0, sizeof(actual));
  actual_size = io5_conv_get(conv, actual, sizeof(actual));

  rc = check_data(actual,
                  actual_size,
                  hex_data,
                  sizeof(hex_data) - 1); // exclude trailing '\0'
  if (0 != rc) {
    printf("elliott to hex failed\n");
    return rc;
  }

  if (io5_ok != io5_conv_deallocate(conv)) {
    printf("failed to deallocate conv\n");
    return 1;
  }

  // test Elliott to Elliott encoding

  conv = io5_conv_allocate(io5_mode_elliott, io5_mode_elliott);
  if (NULL == conv) {
    printf("failed to allocate a conv\n");
    return 1;
  }

  n = io5_conv_put(conv, utf8_data, sizeof(utf8_data));
  if (sizeof(utf8_data) != n) {
    printf("error: consumed: %zu of: %zu\n", n, sizeof(utf8_data));
    return 1;
  }

  memset(actual, 0, sizeof(actual));
  actual_size = io5_conv_get(conv, actual, sizeof(actual));

  rc = check_data(actual, actual_size, utf8_data, sizeof(utf8_data));
  if (0 != rc) {
    printf("elliott to elliott failed\n");
    return rc;
  }

  if (io5_ok != io5_conv_deallocate(conv)) {
    printf("failed to deallocate conv\n");
    return 1;
  }

  // done

  return 0;
}

int main(int argc, char *argv[]) {

  int rc = check_locale();
  if (0 != rc) {
    return rc;
  }

  // do all tests
  rc = do_test();

  // result
  return rc;
}

// write_test.c

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "check.h"
#include "io5.h"
#include "structs.h"

static int write_file(const char *file_name,
                      io5_mode_t mode,
                      const void *data,
                      size_t data_size) {

  io5_file_t *file = io5_file_allocate();
  if (NULL == file) {
    printf("failed to allocate a file\n");
    return 1;
  }

  // ensure file does not exist
  unlink(file_name);

  // create write only file
  io5_error_t rc = io5_file_create(file, file_name, mode);
  if (io5_ok != rc) {
    printf("create failed error: %d\n", rc);
    return 1;
  }

  // this must fail for write mode file
  {
    uint8_t buffer[16]; // dummy buffer
    ssize_t n = io5_file_read(file, buffer, sizeof(buffer));
    if (n >= 0) {
      printf("unexpected success, read: %zd\n", n);
      return 1;
    }
  }

  //  n = io5_file_write(io, all_chars, sizeof(all_chars));
  ssize_t n = io5_file_write(file, data, data_size);
  if (data_size != n) {
    printf("write failed, only wrote: %zd\n", n);
    return 1;
  }

  rc = io5_file_close(file);
  if (io5_ok != rc) {
    printf("close failed error: %d\n", rc);
    return 1;
  }

  rc = io5_file_deallocate(file);
  if (io5_ok != rc) {
    printf("deallocate failed error: %d\n", rc);
    return 1;
  }
  return 0;
}

static size_t
raw_read_file(const char *file_name, void *buffer, size_t buffer_size) {

  // read back the temp file
  FILE *in = fopen(file_name, "r");
  if (NULL == in) {
    return 0;
  }

  memset(buffer, 0, buffer_size);

  size_t count = fread(buffer, 1, buffer_size, in);
  fclose(in);

  return count;
}

int do_test(const char *file_name) {

  const uint8_t lines[] = {
    0x00, 0x00, 0x1c, 0x1d, 0x1e, // CR LF

    0x1f, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, // letters
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e,

    0x1b, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, // figures
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e,

    0x00, 0x00,
  };
  const uint8_t utf8_expected[] = "\0\0 \r\n"                       // control
                                  "abcdefghijklmnopqrstuvwxyz \r\n" // letters
                                  "12*4<=78',+:-.>0()3?56/@9→ \r\n" // figures
                                  "\0"; // also extra '\0' after end
  const uint8_t hex_expected[] =
    "00\n00\n1c\n1d\n1e\n"
    "1f\n01\n02\n03\n04\n05\n06\n07\n08\n09\n0a\n0b\n"
    "0c\n0d\n0e\n0f\n10\n11\n12\n13\n14\n15\n16\n17\n"
    "18\n19\n1a\n1c\n1d\n1e\n"
    "1b\n01\n02\n03\n04\n05\n06\n07\n08\n09\n0a\n0b\n"
    "0c\n0d\n0e\n0f\n10\n11\n12\n13\n14\n15\n16\n17\n"
    "18\n19\n1a\n1c\n1d\n1e\n00\n00\n";

  int rc = 0;
  uint8_t actual[1024];

  // test hex5 encoding

  rc = write_file(file_name, io5_mode_hex5, lines, sizeof(lines));
  if (0 != rc) {
    printf("write_file: hex encoding failed\n");
    return rc;
  }

  size_t actual_size = raw_read_file(file_name, actual, sizeof(actual));
  if (0 == actual_size) {
    printf("failed to read temp file\n");
    return 1;
  }

  rc = check_data(actual,
                  actual_size,
                  hex_expected,
                  sizeof(hex_expected) - 1); // exclude trailing NUL
  if (0 != rc) {
    printf("check_data: hex encoding failed\n");
    return rc;
  }

  // test Elliott encoding

  rc = write_file(file_name, io5_mode_elliott, lines, sizeof(lines));
  if (0 != rc) {
    printf("write_file: elliott encoding failed\n");
    return rc;
  }

  actual_size = raw_read_file(file_name, actual, sizeof(actual));
  if (0 == actual_size) {
    printf("failed to read temp file\n");
    return 1;
  }

  rc = check_data(actual, actual_size, utf8_expected, sizeof(utf8_expected));
  if (0 != rc) {
    printf("check_data: elliott encoding failed\n");
    return rc;
  }

  return 0;
}

int main(int argc, char *argv[]) {

  int rc = check_locale();
  if (0 != rc) {
    return rc;
  }

  const char *file_name = "test.tmp";

  // ensure file does not exist
  unlink(file_name);

  // do all tests
  rc = do_test(file_name);

  // clean up
  unlink(file_name);

  // result
  return rc;
}

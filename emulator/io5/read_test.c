// read_test.c

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "check.h"
#include "io5.h"
#include "structs.h"

static ssize_t read_file(const char *file_name,
                         io5_mode_t mode,
                         void *data,
                         size_t data_size) {

  io5_file_t *io = io5_file_allocate();
  if (NULL == io) {
    printf("failed to allocate an io\n");
    return -1;
  }

  // open file for reading
  io5_error_t rc = io5_file_open(io, file_name, mode);
  if (io5_ok != rc) {
    printf("open failed error: %d\n", rc);
    return -1;
  }

  // this must fail for read mode file
  {
    uint8_t buffer[16]; // dummy buffer
    ssize_t n = io5_file_write(io, buffer, sizeof(buffer));
    if (n >= 0) {
      printf("unexpected success, wrote: %zd\n", n);
      return -1;
    }
  }

  ssize_t n = io5_file_read(io, data, data_size);
  if (n < 0) {
    printf("read failed, I/O error: %zd into: %zd\n", n, data_size);
    return -1;
  }
  if ((size_t)(n) > data_size) {
    printf("read failed, buffer overrun: %zd into: %zd\n", n, data_size);
    return -1;
  }

  rc = io5_file_close(io);
  if (io5_ok != rc) {
    printf("close failed error: %d\n", rc);
    return -1;
  }

  rc = io5_file_deallocate(io);
  if (io5_ok != rc) {
    printf("deallocate failed error: %d\n", rc);
    return -1;
  }
  return n;
}

static ssize_t
raw_write_file(const char *file_name, const void *buffer, size_t buffer_size) {

  // ensure file does not exist
  unlink(file_name);

  // create a test file for reader
  FILE *out = fopen(file_name, "wbx");
  if (NULL == out) {
    return -1;
  }

  size_t count = fwrite(buffer, 1, buffer_size, out);
  fclose(out);

  return (ssize_t)(count);
}

static int do_test(const char *file_name) {

  const uint8_t lines[] =             // string of all possible sets
    "\0\0 \r\n"                       // control chars
    "abcdefghijklmnopqrstuvwxyz \r\n" // Letters
    "12*4$=78',+:-.%0()3?56/@9# \r\n" // Figures ASCII
    "12*4$=78',+:-.%0()3?56/@9£ \r\n" // Figures Elliott (UTF-8)
    "12*4<=78',+:-.>0()3?56/@9→ \r\n" // Figures H-Code (UTF-8)
    "12*4$=78;,+:-.%0()3´56/@9` \r\n" // Figures Algol60 (ASCII)
    "abcdefghijklmnopqrstuvwxyz \r\n" // Letters
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ \r\n" // Letters
    "\0";                             // extra '\0' as terminator

  const uint8_t lines_expected[] = {
    0x00, 0x00, 0x1c, 0x1d, 0x1e, // CR LF

    0x1f, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, // letters
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e,

    0x1b, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, // figures
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e,

    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, // figures
    0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e,

    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, // figures
    0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e,

    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, // figures
    0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e,

    0x1f, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, // letters
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e,

    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, // letters
    0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e,

    0x00, 0x00,
  };

  const uint8_t hex[] = "00\n00\n1c\n1d\n1e\n"

                        "1f\n01\n02\n03\n04\n05\n06\n07\n08\n09\n0a\n0b\n"
                        "0c\n0d\n0e\n0f\n10\n11\n12\n13\n14\n15\n16\n17\n"
                        "18\n19\n1a\n1c\n1d\n1e\n"

                        "1b\n01\n02\n03\n04\n05\n06\n07\n08\n09\n0a\n0b\n"
                        "0c\n0d\n0e\n0f\n10\n11\n12\n13\n14\n15\n16\n17\n"
                        "18\n19\n1a\n1c\n1d\n1e\n"

                        "01\n02\n03\n04\n05\n06\n07\n08\n09\n0a\n0b\n"
                        "0c\n0d\n0e\n0f\n10\n11\n12\n13\n14\n15\n16\n17\n"
                        "18\n19\n1a\n1c\n1d\n1e\n"

                        "01\n02\n03\n04\n05\n06\n07\n08\n09\n0a\n0b\n"
                        "0c\n0d\n0e\n0f\n10\n11\n12\n13\n14\n15\n16\n17\n"
                        "18\n19\n1a\n1c\n1d\n1e\n"

                        "01\n02\n03\n04\n05\n06\n07\n08\n09\n0a\n0b\n"
                        "0c\n0d\n0e\n0f\n10\n11\n12\n13\n14\n15\n16\n17\n"
                        "18\n19\n1a\n1c\n1d\n1e\n"

                        "1f\n01\n02\n03\n04\n05\n06\n07\n08\n09\n0a\n0b\n"
                        "0c\n0d\n0e\n0f\n10\n11\n12\n13\n14\n15\n16\n17\n"
                        "18\n19\n1a\n1c\n1d\n1e\n"

                        "01\n02\n03\n04\n05\n06\n07\n08\n09\n0a\n0b\n"
                        "0c\n0d\n0e\n0f\n10\n11\n12\n13\n14\n15\n16\n17\n"
                        "18\n19\n1a\n1c\n1d\n1e\n"

                        "00\n00\n";

  int rc = 0;
  uint8_t actual[1024];

  // test hex5 encoding

  ssize_t actual_size = raw_write_file(file_name, hex, sizeof(hex));
  if (actual_size <= 0) {
    printf("failed to create temp file\n");
    return 1;
  }

  actual_size = read_file(file_name, io5_mode_hex5, actual, sizeof(actual));
  if (actual_size <= 0) {
    return 1;
  }

  rc = check_data(actual,
                  (size_t)(actual_size),
                  lines_expected,
                  sizeof(lines_expected)); // exclude trailing NUL
  if (0 != rc) {
    printf("hex encoding read failed\n");
    return rc;
  }

  // test Elliott encoding

  actual_size = raw_write_file(file_name, lines, sizeof(lines));
  if (actual_size <= 0) {
    printf("failed to create temp file\n");
    return 1;
  }

  actual_size = read_file(file_name, io5_mode_elliott, actual, sizeof(actual));
  if (actual_size <= 0) {
    return 1;
  }

  rc = check_data(actual,
                  (size_t)(actual_size),
                  lines_expected,
                  sizeof(lines_expected)); // exclude trailing NUL
  if (0 != rc) {
    printf("elliott encoding read failed\n");
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
  //  unlink(file_name);

  // result
  return rc;
}

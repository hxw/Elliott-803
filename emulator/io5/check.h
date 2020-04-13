// check.h

#if !defined(CHECK_H)
#define CHECK_H

#include <locale.h>
#include <stdint.h>

static int check_data(const uint8_t *actual, size_t actual_size,
                      const uint8_t *expected, size_t expected_size) {

  if (expected_size != actual_size ||
      0 != memcmp(expected, actual, expected_size)) {
    printf("incorrect actual: %zu  expected: %zu\n", actual_size,
           expected_size);
    size_t max = expected_size;
    if (actual_size > max) {
      max = actual_size;
    }
    for (size_t i = 0; i < max; ++i) {
      if (i >= expected_size) {
        printf("%4zu: %02x  ---\n", i, actual[i]);
      } else if (i >= actual_size) {
        printf("%4zu: ---  %02x\n", i, expected[i]);
      } else if (actual[i] != expected[i]) {
        printf("%4zu: %02x  %02x\n", i, actual[i], expected[i]);
      }
    }
    return 1;
  }
  return 0;
}

static int check_locale(void) {
  // ensure locale is properly set up
  const char *l = setlocale(LC_ALL, "");

  // check that UTF-8 is setup
  char *p = strchrnul(l, '.');

  if (0 != strcmp(".UTF-8", p)) {
    printf("locale: \"%s\"  is not UTF-8\n", l);
    return 1;
  }
  return 0;
}

#endif

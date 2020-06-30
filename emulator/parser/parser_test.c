// parser_test.c

#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "parser.h"

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

int do_test(void) {

  typedef struct {
    const wchar_t *s;
    const wchar_t *word[20];
  } test_t;

  const test_t items[] = {
    {
      .s = L" this is a test ",
      .word = {L"this", L"is", L"a", L"test", NULL},
    },
    {
      .s = L" this is å test ",
      .word = {L"this", L"is", L"å", L"test", NULL},
    },
    {
      .s = L" this is å→£½ test ",
      .word = {L"this", L"is", L"å→£½", L"test", NULL},
    },
    {
      .s = L" this `is a´ test ",
      .word = {L"this", L"is a", L"test", NULL},
    },
    {
      .s = L" this 'is a' test ",
      .word = {L"this", L"is a", L"test", NULL},
    },
    {
      .s = L" this \"is a\" test ",
      .word = {L"this", L"is a", L"test", NULL},
    },
    {
      .s = L" this is\" a\"no'th'er test ",
      .word = {L"this", L"is another", L"test", NULL},
    },
    {
      .s = L" this `is\" a\"no'th'er´ test ",
      .word = {L"this", L"is\" a\"no'th'er", L"test", NULL},
    },
    {
      .s = L" non-terminated \"quote goes to end ",
      .word = {L"non-terminated", L"quote goes to end ", NULL},
    },
    {
      .s = L" non-terminated 'quote goes to end ",
      .word = {L"non-terminated", L"quote goes to end ", NULL},
    },
    {
      .s = L" non-terminated `quote goes to end ",
      .word = {L"non-terminated", L"quote goes to end ", NULL},
    },

    {
      NULL,
      {},
    },
  };

  wchar_t buffer[1024];

  for (size_t i = 0; NULL != items[i].s; ++i) {

    size_t n = wcslcpy(buffer, items[i].s, sizeof(buffer));
    if (n >= sizeof(buffer)) {
      printf("%2zu: failed to copy source string buffer[%zu] too small need "
             ":%zu\n",
             i,
             sizeof(buffer),
             n);
      return 1;
    }

    wchar_t *str = buffer;
    for (size_t j = 0; NULL != items[i].word[j]; ++j) {

      const wchar_t *token = parser_get_token(&str);

      if (0 != wcscmp(items[i].word[j], token)) {
        printf(
          "%2zu: word[%2zu] mismatch: actual: \"%ls\"  expected: \"%ls\"\n",
          i,
          j,
          token,
          items[i].word[j]);
        return 1;
      }
    }

    const wchar_t *null_token = parser_get_token(&str);
    if (NULL != null_token) {
      printf("%2zu: word[final] mismatch: actual: \"%ls\"  expected: <NULL>\n",
             i,
             null_token);
      return 1;
    }
  }
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

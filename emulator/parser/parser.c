// parser.c

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "parser.h"

// parse one "word" from a string pointed to by *p
// update *p to be ready for next token
// words are space separated or quotes using "" '' or `´.
// quotes do not nest and end of string terminates the last word
// return
//   string is part of the original string
//   and is modified to remove quotes and is L'\0' terminates
const wchar_t *parser_get_token(wchar_t **p) {

  if (NULL == p) {
    return NULL;
  }
  if (NULL == *p) {
    return NULL;
  }
  wchar_t *s = *p;

  // skip leading white space
  while (iswspace(*s)) {
    ++s;
  }

  wchar_t *start = s;
  wchar_t *copy = s;
  wchar_t end = L'\0';
  bool check_space = true;

  while (L'\0' != *s) {

    if (check_space) {
      if (L'"' == *s || L'\'' == *s) {
        end = *s;
        check_space = false;
        ++s;
        continue;
      } else if (L'`' == *s) {
        end = L'´';
        ++s;
        check_space = false;
        continue;
      }

      if (iswspace(*s)) {
        break;
      }
    } else if (end == *s) {
      ++s;
      check_space = true;
      continue;
    }
    if (copy != s) {
      *copy = *s;
    }
    ++s;
    ++copy;
  }

  // update for next token
  if (L'\0' != *s) {
    ++s;
  }
  *p = s;

  *copy = L'\0';
  return copy == start ? NULL : start;
}

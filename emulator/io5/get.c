// write.c

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "io5.h"
#include "structs.h"

// read internal buffer and convert as "to" "characters"
// returns:
//   +N   number of characters returned (maybe zero)
size_t io5_conv_get(io5_conv_t *conv, uint8_t *buffer, size_t length) {

  size_t n = 0;
  while (n < length && conv->put != conv->get) {

    int c = conv->buffer[conv->get];

    switch (conv->to) {
    default:
    case io5_mode_hex5:
      c &= 0x1f;
      __attribute__((fallthrough));
    case io5_mode_hex8:
      c &= 0xff;
      char temp[4]; // "XX\n\0" = 4 bytes
      size_t k = (size_t)snprintf(temp, sizeof(temp), "%02x\n", c);

      if (n + k > length) {
        // insufficient space
        return n;
      }
      memcpy(&buffer[n], temp, k);
      n += k;
      break;

    case io5_mode_binary:
      buffer[n++] = (uint8_t)(c);
      break;

    case io5_mode_elliott: {
      c &= 0x1f;
      if (27 == c) {
        conv->shift_to = shift_figures;
        break;
      } else if (31 == c) {
        conv->shift_to = shift_letters;
        break;
      } else if (0 == c) {
        buffer[n++] = 0;
      } else if (28 == c) {
        buffer[n++] = ' ';
      } else if (29 == c) {
        buffer[n++] = '\r';
      } else if (30 == c) {
        buffer[n++] = '\n';
      } else if (shift_figures == conv->shift_to) {
        const char *s = "";
        switch (c) {
        case 1:
          s = "1";
          break;
        case 2:
          s = "2";
          break;
        case 3:
          s = "*";
          break;
        case 4:
          s = "4";
          break;
        case 5:
          // s = "$";
          s = "<";
          break;
        case 6:
          s = "=";
          break;
        case 7:
          s = "7";
          break;
        case 8:
          s = "8";
          break;
        case 9:
          s = "'";
          break;
        case 10:
          s = ",";
          break;
        case 11:
          s = "+";
          break;
        case 12:
          s = ":";
          break;
        case 13:
          s = "-";
          break;
        case 14:
          s = ".";
          break;
        case 15:
          // s = ">";
          s = ">";
          break;
        case 16:
          s = "0";
          break;
        case 17:
          s = "(";
          break;
        case 18:
          s = ")";
          break;
        case 19:
          s = "3";
          break;
        case 20:
          s = "?";
          break;
        case 21:
          s = "5";
          break;
        case 22:
          s = "6";
          break;
        case 23:
          s = "/";
          break;
        case 24:
          s = "@";
          break;
        case 25:
          s = "9";
          break;
        case 26:
          // s = "#";
          // s = "£";
          s = "→";
          break;
        default:
          break;
        }
        k = strlen(s);
        if (n + k > length) {
          // insufficient space
          return n;
        }
        memcpy(&buffer[n], s, k);
        n += k;
      } else {
        buffer[n++] = (uint8_t)(c + 'a' - 1);
      }
      break;
    }
    }

    // one internal buffer character was successfully processed
    size_t next = conv->get + 1;
    if (next >= sizeof(conv->buffer)) {
      next = 0;
    }
    conv->get = next;
  }
  return n;
}

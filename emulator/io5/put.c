// read.c

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "io5.h"
#include "structs.h"

// process hex encoded data
//
// XX…\n        - two hex digits
// #skip…\n     - set skip mode only #s or #S
// #endskip…\n  - end skip mode only #e or #E
static int process_hex_data(io5_conv_t *conv, uint8_t c) {
  int b = -1; // negative means no data

  switch (conv->from_state) {
  default:
  case state_begin: // scan for hex digit
    if ('#' == c) {
      conv->from_state = state_hash;
    } else if ('\r' == c || '\n' == c || ' ' == c || '\t' == c) {
    } else if (isxdigit(c)) {
      conv->from_state = state_hex;
      conv->from_byte[0] = c;
    }
    break;

  case state_hex: // scan for second hex digit
    if (isxdigit(c)) {
      conv->from_byte[1] = c;
      conv->from_byte[2] = '\0';
      unsigned int n = 0;
      sscanf((const char *)conv->from_byte, "%x", &n);
      b = (int)(n);
    }
    conv->from_state = state_eol;
    break;

  case state_eol:
    if ('\r' == c || '\n' == c) {
      conv->from_state = state_begin;
    }
    break;

  case state_hash:
    if ('s' == c || 'S' == c) {
      conv->from_state = state_skip;
    } else if ('e' == c || 'E' == c) {
      conv->from_state = state_eol;
    }
    break;

  case state_skip:
    if ('\r' == c || '\n' == c) {
      conv->from_state = state_skip_hash;
    }
    break;

  case state_skip_hash:
    if ('#' == c) {
      conv->from_state = state_hash;
    }
    break;
  }
  return b;
}

// process UTF-8 encoded data
// from man 5 utf8:
// 1  [0x00000000 - 0x0000007f] [00000000.0bbbbbbb] ->
//         0bbbbbbb
// 2  [0x00000080 - 0x000007ff] [00000bbb.bbbbbbbb] ->
//         110bbbbb, 10bbbbbb
// 3  [0x00000800 - 0x0000ffff] [bbbbbbbb.bbbbbbbb] ->
//         1110bbbb, 10bbbbbb, 10bbbbbb
// 4  [0x00010000 - 0x001fffff] [00000000.000bbbbb.bbbbbbbb.bbbbbbbb] ->
//         11110bbb, 10bbbbbb, 10bbbbbb, 10bbbbbb
// 5  [0x00200000 - 0x03ffffff] [000000bb.bbbbbbbb.bbbbbbbb.bbbbbbbb] ->
//         111110bb, 10bbbbbb, 10bbbbbb, 10bbbbbb, 10bbbbbb
// 6  [0x04000000 - 0x7fffffff] [0bbbbbbb.bbbbbbbb.bbbbbbbb.bbbbbbbb] ->
//         1111110b, 10bbbbbb, 10bbbbbb, 10bbbbbb, 10bbbbbb, 10bbbbbb
//
static wint_t process_utf8_data(io5_conv_t *conv, uint8_t c) {
  wint_t b = -1; // negative means no data

  switch (conv->from_state) {
  default:
  case state_begin: // ASCII or UTF-8 start
    if (c < 0x80 || 0xff == c || 0xfe == c) {
      b = c;
    } else { // UTF-8 start
      conv->from_wchar = c;
      c <<= 1;
      int mask = 0x3f;
      int i = 0; // count of number of extension bytes
      while (0 != (c & 0x80)) {
        c <<= 1;
        mask >>= 1;
        ++i;
      }
      conv->from_wchar &= mask;
      // select one of the five states (in reverse order)
      conv->from_state = (state_t)(state_utf8_5 + 5 - i);
    }
    break;

  case state_utf8_5:
  case state_utf8_4:
  case state_utf8_3:
  case state_utf8_2:
    ++conv->from_state;
    conv->from_wchar = (conv->from_wchar << 6) | (c & 0x3f);
    break;

  case state_utf8_1:
    conv->from_wchar = (conv->from_wchar << 6) | (c & 0x3f);
    b = conv->from_wchar;
    conv->from_state = state_begin;
    break;
  }
  return b;
}

// send "characters" encoded as "from" to internal buffer
// returns:
//   N   number of characters consumed (maybe zero)
size_t io5_conv_put(io5_conv_t *conv, const uint8_t *buffer, size_t length) {

  shift_t current_shift = shift_null;

  size_t n = 0;
  for (; n < length; ++n, ++buffer) {

    int free_bytes = 0;
    // determine if free space in buffer
    size_t next = conv->put + 1;
    if (next >= sizeof(conv->buffer)) {
      next = 0;
    }
    if (next == conv->get) {
      break; // no space in buffer
    }
    ++free_bytes;

    // check if second free byte (for the case of a shift change)
    size_t next2 = next + 1;
    if (next2 >= sizeof(conv->buffer)) {
      next2 = 0;
    }
    if (next2 != conv->get) {
      ++free_bytes;
    }

    uint8_t c = *buffer;
    int b = -1; // negative means no data byte

    int mask = 0xff; // assume hex 8 bit

    switch (conv->from) {
    default:
    case io5_mode_hex5:
      mask = 0x1f; // reset mask to 5 bits and same as hex8
    case io5_mode_hex8:
      b = process_hex_data(conv, c);
      break;

    case io5_mode_binary:
      b = c;
      break;

    case io5_mode_elliott: {

      shift_t shift = shift_figures;
      wint_t w = process_utf8_data(conv, c);
      if (w < 0) {
        break;
      }

      if (w >= L'a' && w <= L'z') {
        b = w - L'a' + 1;
        shift = shift_letters;
      } else if (w >= L'A' && w <= L'Z') {
        b = w - L'A' + 1;
        shift = shift_letters;
      } else { // handle remaining non-letters
        switch (w) {
        case L'\0':
          shift = current_shift;
          b = 0;
          break;
        case L'1':
          b = 1;
          break;
        case L'2':
          b = 2;
          break;
        case L'*':
          b = 3;
          break;
        case L'4':
          b = 4;
          break;
        case L'$':
        case L'<':
          b = 5;
          break;
        case L'=':
          b = 6;
          break;
        case L'7':
          b = 7;
          break;
        case L'8':
          b = 8;
          break;
        case L';': // special for Algol60
        case L'\'':
          b = 9;
          break;
        case L',':
          b = 10;
          break;
        case L'+':
          b = 11;
          break;
        case L':':
          b = 12;
          break;
        case L'-':
          b = 13;
          break;
        case L'.':
          b = 14;
          break;
        case L'%':
        case L'>':
          b = 15;
          break;
        case L'0':
          b = 16;
          break;
        case L'(':
          b = 17;
          break;
        case L')':
          b = 18;
          break;
        case L'3':
          b = 19;
          break;
        case L'´': // special for Algol60 (close quote)
        case L'?':
          b = 20;
          break;
        case L'5':
          b = 21;
          break;
        case L'6':
          b = 22;
          break;
        case L'/':
          b = 23;
          break;
        case L'@':
          b = 24;
          break;
        case L'9':
          b = 25;
          break;
        case L'`': // special for Algol60 (open quote)
        case L'#':
        case L'£': // UTF-8 handled above
        case L'→': // ...
          b = 26;
          break;
        case L' ':
          shift = current_shift;
          b = 28;
          break;
        case L'\r':
          shift = current_shift;
          b = 29;
          break;
        case L'\n':
          shift = current_shift;
          b = 30;
          break;
        default:
          shift = current_shift;
          b = 0;
        }
      }
      if (shift != current_shift) {
        // if change of shift and only one available byte
        // then delay shift until next read
        if (free_bytes < 2) {
          break;
        }

        current_shift = shift; // preserve current shift
        if (shift_letters == shift) {
          conv->buffer[conv->put] = 31; // letter shift
          conv->put = next;
          next = next2;
        } else {
          conv->buffer[conv->put] = 27; // figure shift
          conv->put = next;
          next = next2;
        }
      }
      break;
    }
    }

    if (b >= 0) {
      conv->buffer[conv->put] = (uint8_t)(b & mask);
      conv->put = next;
    }
  }
  return n;
}

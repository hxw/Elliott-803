// parser.h

#if !defined(PARSER_H)
#define PARSER_H

#include <stdint.h>
#include <wchar.h>

const wchar_t *parser_get_token(wchar_t **p);

#endif

// commands.c

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "commands.h"
#include "cpu/elliott803.h"
#include "parser/parser.h"

#if !defined(SizeOfArray)
#define SizeOfArray(a) (sizeof(a) / sizeof((a)[0]))
#endif

// lookup file mode

typedef struct {
  const wchar_t *name;
  io5_mode_t mode;
} file_mode_t;

const file_mode_t modes[] = {
    {L"hex5", io5_mode_hex5},       {L"h5", io5_mode_hex5},
    {L"hex8", io5_mode_hex8},       {L"h8", io5_mode_hex8},
    {L"binary", io5_mode_binary},   {L"bin", io5_mode_binary},
    {L"elliott", io5_mode_elliott}, {L"utf-8", io5_mode_elliott},
    {L"utf8", io5_mode_elliott},
};

static io5_mode_t string_to_mode(const wchar_t *w) {
  for (size_t i = 0; i < SizeOfArray(modes); ++i) {
    if (0 == wcscasecmp(modes[i].name, w)) {
      return modes[i].mode;
    }
  }
  return io5_mode_invalid;
}

static void command_exit(commands_t *cmd, const wchar_t *name, wchar_t **ptr) {
  cmd->exit_program = true;
}

static void command_wait(commands_t *cmd, const wchar_t *name, wchar_t **ptr) {

  cmd->wait = true;
  cmd->wait_delay = 0;

  const wchar_t *w = parser_get_token(ptr);
  if (NULL != w) {
    long seconds = wcstol(w, NULL, 10);
    if (seconds > 0 && seconds < 3600) {
      cmd->wait_delay = (int)(seconds);
    }
  }
}

static void command_screen(commands_t *cmd, const wchar_t *name,
                           wchar_t **ptr) {
  const wchar_t *w = parser_get_token(ptr);
  if (NULL != w) {
    long s = wcstol(w, NULL, 10);
    if (s > 0 && s < 4) {
      cmd->screen = (int)(s);
    }
  }
}

static void command_list(commands_t *cmd, const wchar_t *name, wchar_t **ptr) {

  long address = 0;
  long count = 0;

  const wchar_t *w = parser_get_token(ptr);
  if (NULL != w) {
    address = wcstol(w, NULL, 10);
  }
  w = parser_get_token(ptr);
  if (NULL != w) {
    count = wcstol(w, NULL, 10);
  }
  if (address <= 0) {
    address = 0;
  }
  if (count <= 0) {
    count = 10;
  }
  if (count > 20) {
    count = 20;
  }

  char packet[256];

  for (int i = 0; i < count; ++i) {
    memset(packet, 0, sizeof(packet));
    int n = snprintf(packet, sizeof(packet), "mr %ld", address + i);
    elliott803_send(cmd->proc, packet, n);
  }
}

static void command_memory_write(commands_t *cmd, const wchar_t *name,
                                 wchar_t **ptr) {

  long address = -1;

  const wchar_t *w = parser_get_token(ptr);
  if (NULL != w) {
    address = wcstol(w, NULL, 10);
  }

  char packet[256];

  memset(packet, 0, sizeof(packet));
  int n = snprintf(packet, sizeof(packet), "mw %ld %ls", address, *ptr);
  elliott803_send(cmd->proc, packet, n);
}

static void command_reset(commands_t *cmd, const wchar_t *name, wchar_t **ptr) {

  const wchar_t *w = parser_get_token(ptr);
  if (NULL != w && 0 == wcscasecmp(L"run", w)) {
    elliott803_send(cmd->proc, "reset run", 10);
  } else {
    elliott803_send(cmd->proc, "reset", 6);
  }
}

static void command_run(commands_t *cmd, const wchar_t *name, wchar_t **ptr) {

  long address = 0;
  const wchar_t *w = parser_get_token(ptr);
  if (NULL == w) {
    elliott803_send(cmd->proc, "cont", 5);
    return;
  }

  wchar_t *end = NULL;
  address = wcstol(w, &end, 10);

  if (address <= 0) {
    address = 0;
  }
  bool half = false;
  if (0 == wcscasecmp(end, L".0")) {
    half = false;
  } else if (0 == wcscasecmp(end, L".5")) {
    half = true;
  } else if (L'\0' != *end) {
    cmd->error = wcsdup(L"error: invalid address");
  }

  char packet[256];
  memset(packet, 0, sizeof(packet));
  int n = snprintf(packet, sizeof(packet), "run %ld%s", address,
                   half ? ".5" : ".0");
  elliott803_send(cmd->proc, packet, n);
}

static void command_stop(commands_t *cmd, const wchar_t *name, wchar_t **ptr) {
  elliott803_send(cmd->proc, "stop", 5);
}

static void command_registers(commands_t *cmd, const wchar_t *name,
                              wchar_t **ptr) {
  elliott803_send(cmd->proc, "status", 7);
}

static void command_hello(commands_t *cmd, const wchar_t *name, wchar_t **ptr) {
  long address = 0;
  long punch = 0;

  const wchar_t *w = parser_get_token(ptr);
  if (NULL != w) {
    address = wcstol(w, NULL, 10);
  }
  w = parser_get_token(ptr);
  if (NULL != w) {
    punch = wcstol(w, NULL, 10);
  }
  if (address <= 0) {
    address = 4096;
  }
  switch (punch) {
  case 2:
    punch = 2048;
    break;
  case 3:
    punch = 4096;
    break;
  default:
    punch = 0;
    break;
  }

  const char *hello_init[] = {
      "mw %ld  26    4 : 02    4", //
      "mw %ld  55    2 : 22    4", //
      "mw %ld  04    4 : 21    4", // set [4] = -5
  };

  const char *hello_punch_two = "mw %ld  74 %4d : 74 %4d";

  struct {
    char c1;
    char c2;
  } hello_chars[] = {
      {27, 31}, // FS LS
      {29, 30}, // CR LF
      {8, 5},   // hello, world.
      {12, 12}, //
      {15, 27}, //
      {10, 28}, //
      {31, 23}, //
      {15, 18}, //
      {12, 4},  //
      {27, 14}, //
  };

  const char *hello_loop[] = {
      "mw %ld  22    4 : 30    4",  //
      "mw %ld  42 %1$4d : 40%2$4d", //
  };

  char packet[256];
  for (size_t i = 0; i < SizeOfArray(hello_init); ++i) {
    memset(packet, 0, sizeof(packet));
    int n = snprintf(packet, sizeof(packet), hello_init[i], address);
    elliott803_send(cmd->proc, packet, n);
    ++address;
  }

  int loop_address = address;

  for (size_t i = 0; i < SizeOfArray(hello_chars); ++i) {
    memset(packet, 0, sizeof(packet));
    int n = snprintf(packet, sizeof(packet), hello_punch_two, address,
                     hello_chars[i].c1 + punch, hello_chars[i].c2 + punch);
    elliott803_send(cmd->proc, packet, n);
    ++address;
  }

  for (size_t i = 0; i < SizeOfArray(hello_loop); ++i) {
    memset(packet, 0, sizeof(packet));
    int n =
        snprintf(packet, sizeof(packet), hello_loop[i], address, loop_address);
    elliott803_send(cmd->proc, packet, n);
    ++address;
  }
}

// reader unit [mode] file
// if mode is absent then assume "hex5"
static void command_reader(commands_t *cmd, const wchar_t *name,
                           wchar_t **ptr) {
  long unit = 0;

  const wchar_t *w = parser_get_token(ptr);
  if (NULL != w) {
    unit = wcstol(w, NULL, 10);
  }
  if (unit < 1 || unit > 2) {
    cmd->error = wcsdup(L"error: invalid unit");
    return;
  }
  w = parser_get_token(ptr);
  if (NULL == w) {
    cmd->error = wcsdup(L"error: missing filename");
    return;
  }

  commands_io_t io = 1 == unit ? commands_reader_1 : commands_reader_2;
  io5_mode_t mode = io5_mode_hex5;

  const wchar_t *w1 = parser_get_token(ptr);
  if (NULL != w1) {
    mode = string_to_mode(w);
    if (0 == wcscasecmp(w1, L"close")) {
      io5_file_close(cmd->file[io]);
      return;
    } else if (io5_mode_invalid == mode) {
      cmd->error = wcsdup(L"error: mode is invalid");
      return;
    }
    w = w1;
  }

  char filename[1024];
  size_t n = snprintf(filename, sizeof(filename), "%ls", w);
  if (n >= sizeof(filename) - 1) {
    cmd->error = wcsdup(L"error: filename is too long");
    return;
  }

  if ('/' == filename[0] || ('.' == filename[0] && '/' == filename[1]) ||
      ('.' == filename[0] && '.' == filename[1] && '/' == filename[2])) {

    // absolute/relative path
    if (io5_ok == io5_file_open(cmd->file[io], filename, mode)) {
      return;
    }

  } else { // search the path
    char *path = NULL;
    char *p = getenv("E803_TAPE_DIR");
    if (NULL == p) {
      path = malloc(1);
      if (NULL == path) {
        cmd->error = wcsdup(L"error: malloc failed");
        return;
      }
      *path = '\0'; // just the current directory will be searched
    } else {
      size_t len = strlen(p) + 2; // colon prefix and trailing '\0'
      path = malloc(len);
      if (NULL == path) {
        cmd->error = wcsdup(L"error: malloc failed");
        return;
      }
      path[0] = ':'; // current directory will be searched
      path[1] = '\0';
      strlcat(path, p, len); // append rest of env
    }

    p = path;
    const char *q = NULL;
    while (NULL != (q = strsep(&p, ":"))) {
      const char *s = '\0' == *q ? "" : "/";
      size_t n = snprintf(filename, sizeof(filename), "%s%s%ls", q, s, w);
      if (n >= sizeof(filename) - 1) {
        cmd->error = wcsdup(L"error: filename is too long");
        return;
      }
      if (io5_ok == io5_file_open(cmd->file[io], filename, mode)) {
        return;
      }
    }
  }
  cmd->error = wcsdup(L"error: file not found");
}

// punch unit [mode] file
// if mode is absent then assume "hex5"
static void command_punch(commands_t *cmd, const wchar_t *name, wchar_t **ptr) {
  long unit = 0;

  const wchar_t *w = parser_get_token(ptr);
  if (NULL != w) {
    unit = wcstol(w, NULL, 10);
  }
  if (unit < 1 || unit > 2) {
    cmd->error = wcsdup(L"error: invalid unit");
    return;
  }
  w = parser_get_token(ptr);
  if (NULL == w) {
    cmd->error = wcsdup(L"error: missing filename");
    return;
  }

  commands_io_t io = 1 == unit ? commands_punch_1 : commands_punch_2;
  io5_mode_t mode = io5_mode_hex5;

  const wchar_t *w1 = parser_get_token(ptr);
  if (NULL != w1) {
    mode = string_to_mode(w);
    if (0 == wcscasecmp(w1, L"close")) {
      io5_file_close(cmd->file[io]);
      return;
    } else if (io5_mode_invalid == mode) {
      cmd->error = wcsdup(L"error: mode is invalid");
      return;
    }
    w = w1;
  }
  char filename[1024];
  size_t n = snprintf(filename, sizeof(filename), "%ls", w);
  if (n >= sizeof(filename) - 1) {
    cmd->error = wcsdup(L"error: filename is too long");
    return;
  }

  if (io5_ok != io5_file_create(cmd->file[io], filename, mode)) {
    cmd->error = wcsdup(L"error: file already exists");
    return;
  }
}

static void command_word_generator(commands_t *cmd, const wchar_t *name,
                                   wchar_t **ptr) {

  if (L'\0' == **ptr) {
    elliott803_send(cmd->proc, "wg", 3);
    return;
  }

  while (iswspace(**ptr)) {
    ++(*ptr);
  }

  if (0 == wcscasecmp(L"msb", *ptr)) {
    elliott803_send(cmd->proc, "wg msb", 7);
    return;
  }

  if (0 == wcscasecmp(L"o2l", *ptr)) {
    elliott803_send(cmd->proc, "wg o2l", 7);
    return;
  }

  if (0 == wcscasecmp(L"lsb", *ptr)) {
    elliott803_send(cmd->proc, "wg lsb", 7);
    return;
  }

  char packet[256];

  memset(packet, 0, sizeof(packet));
  int n = snprintf(packet, sizeof(packet), "wg %ls", *ptr);
  elliott803_send(cmd->proc, packet, n);
}

// help

static void command_help(commands_t *cmd, const wchar_t *name, wchar_t **ptr) {
  const wchar_t *m =
      // clang-format: off
      L"help                  (?)   this message\n"
      L"exit                  (x)   exit emulation\n"
      L"wait                        wait for stop or wg polling\n"
      L"list [ADDR [COUNT]]   (l)   display memory words\n"
      L"mw ADDR CODE|±DEC           write memory word\n"
      L"reset                       reset all regs and stop execution\n"
      L"reset run                   reset all regs and restart from zero\n"
      L"run [ADDR]                  run from address or continue after a stop\n"
      L"stop                        stop execution\n"
      L"regs                  (r)   display registers and status\n"
      L"hello [ADDR [1|2|3]]        load hello world [4096 1]\n"
      L"reader 1|2 [MODE] FILE      attach an existing file to a reader "
      L"(hex5)\n"
      L"punch 1|2 [MODE] FILE       create file and attach to a punch (hex5)\n"
      L"screen 1|2|3|4              select current screen as Fn\n"
      L"wg [msb|o2l|lsb|CODE|±N]    set or display word generator\n"
      // clang-format: on
      ;

  cmd->error = wcsdup(m);
}

// lookup command

typedef struct {
  const wchar_t *name;
  void (*func)(commands_t *cmd, const wchar_t *name, wchar_t **w);
} command_t;

const command_t commands[] = {
    {L"exit", command_exit},       {L"x", command_exit},
    {L"quit", command_exit},       {L"q", command_exit},
    {L"wait", command_wait},       {L"screen", command_screen},

    {L"list", command_list},       {L"l", command_list},
    {L"mw", command_memory_write}, {L"reset", command_reset},
    {L"run", command_run},         {L"stop", command_stop},
    {L"regs", command_registers},  {L"r", command_registers},
    {L"hello", command_hello},     {L"reader", command_reader},
    {L"punch", command_punch},     {L"wg", command_word_generator},

    {L"help", command_help},       {L"?", command_help},
};

void commands_run(commands_t *cmd, wchar_t *buffer, size_t buffer_size) {

  // free any previous message
  if (NULL != cmd->error) {
    free((void *)(cmd->error));
    cmd->error = NULL;
  }

  wchar_t *str = buffer;
  const wchar_t *w = parser_get_token(&str);
  if (NULL == w) {
    return;
  }

  for (size_t i = 0; i < SizeOfArray(commands); ++i) {
    if (0 == wcscasecmp(commands[i].name, w)) {
      buffer[0] = L'\0'; // assume no error
      return commands[i].func(cmd, w, &str);
    }
  }
  cmd->error = wcsdup(L"error: invalid command");
  return;
}

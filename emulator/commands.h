// commands.h

#if !defined(COMMANDS_H)
#define COMMANDS_H

#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

#include "cpu/elliott803.h"
#include "io5/io5.h"

typedef enum {
  commands_reader_1,
  commands_reader_2,
  commands_punch_1,
  commands_punch_2,

  commands_io_count,
} commands_io_t;

// holds data passed to individual commands
typedef struct {
  io5_file_t *file[commands_io_count];
  elliott803_t *proc;
  bool exit_program;
  bool wait;
  const wchar_t *error;
} commands_t;

void commands_run(commands_t *cmd, wchar_t *buffer, size_t buffer_size);

#endif

// processor.h

#if !defined(PROCESSOR_H)
#define PROCESSOR_H 1

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "buffer.h"
#include "constants.h"

// execution modes
typedef enum {
  exec_mode_stop,
  exec_mode_run,
} execution_mode_t;

// busy devices
typedef enum {
  busy_none,
  busy_reader_1, // paper-tape reader 1
  busy_reader_2, // paper-tape reader 2
  busy_reader_3, // teletype
  busy_punch_1,  // paper-tape punch 1
  busy_punch_2,  // paper-tape punch 2
  busy_punch_3,  // teletype
} busy_t;

// size for various internal buffers
static const size_t message_buffer_size = 4096;

// store values are in int64_t
typedef struct elliott803_struct {

  const char *name; // name of this processor instance

  int64_t core_store[memory_size];

  busy_t io_busy;
  bool overflow;
  int64_t accumulator;
  int64_t auxiliary_register;

  int client_socket;      // client side
  int processor_socket;   // processor side
  pthread_t thread;       // execution state
  int64_t word_generator; // cached value received via control channel

  execution_mode_t mode; // stop/run
  int program_counter;   // LSB is half word indicator

  // two paper tape readers and one teleprinter
  buffer_t reader[reader_units];

  // two paper tape punches and one teleprinter
  buffer_t punch[punch_units];

} processor_t;

#endif

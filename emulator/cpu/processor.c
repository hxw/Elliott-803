// processor.c

#include <assert.h>
#include <ctype.h>
#include <errno.h> //strerr
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h> // sleep / usleep / read / write / close

#include "convert.h"
#include "core.h"
#include "cpu803.h"
#include "elliott803.h"
#include "processor.h"

static void *main_loop(void *arg);

// create a processor
elliott803_t *elliott803_create(const char *name) {

  elliott803_t *proc = malloc(sizeof(elliott803_t));
  if (NULL == proc) {
    goto fail;
  }

  memset(proc, 0, sizeof(elliott803_t));
  proc->mode = exec_mode_stop;
  proc->name = strdup(name);

  int sockets[2];
  if (0 != socketpair(PF_UNIX, SOCK_DGRAM, 0, sockets)) {
    printf("error: %d %s\n", errno, strerror(errno));
    goto fail;
  }
  proc->client_socket = sockets[0];
  proc->processor_socket = sockets[1];

  pthread_create(&proc->thread, NULL, main_loop, proc);

  return proc;

fail:
  if (NULL != proc) {
    free(proc);
  }
  return NULL;
}

// destroy the instance
void elliott803_destroy(elliott803_t *proc) {

  // allow destroy on already null pointer
  if (NULL == proc) {
    return;
  }

  // send thread terminate message
  elliott803_send(proc, "terminate", 10);

  // use join to wait for shutdown
  void *rc = NULL;
  pthread_join(proc->thread, &rc);

  if (NULL != proc->name) {
    free((void *)proc->name);
  }

  close(proc->client_socket);
  close(proc->processor_socket);

  memset(proc, 0, sizeof(elliott803_t));
  free(proc);
}

// get the send/receive fd for a select
int elliott803_get_fd(elliott803_t *proc) { return proc->client_socket; }

// send a command
// returns:
//   positive: bytes sent
//   negative: error code
ssize_t
elliott803_send(elliott803_t *proc, const char *buffer, size_t buffer_size) {

  for (;;) {
    errno = 0;
    ssize_t n = send(proc->client_socket, buffer, buffer_size, MSG_DONTROUTE);
    if (-1 == n) {
      // if no buffer space, just retry
      if (ENOBUFS == errno) {
        pthread_yield();
        continue;
      }
      // if interrupted system call, just retry
      if (EINTR != errno) {
        fprintf(stderr, "elliott_send: error: %s\n", strerror(errno));
        abort();
        return errno; // negative value
      }
      continue;
    }
    return n;
  }
}

// receive a response
// returns:
//   positive: bytes received
//   negative: error code
ssize_t
elliott803_receive(elliott803_t *proc, char *buffer, size_t buffer_size) {

  for (;;) {
    errno = 0;
    ssize_t n = recv(
      proc->client_socket, buffer, buffer_size, MSG_WAITALL | MSG_DONTWAIT);
    if (-1 == n) {
      // if interrupted system call, just retry
      if (EINTR != errno) {
        // fprintf(stderr, "select error: %s\n", strerror(errno));
        return errno; // negative value
      }
      continue;
    }

    return n;
  }
}

// reply to client
static ssize_t
reply(elliott803_t *proc, const char *buffer, size_t buffer_size) {

  for (;;) {
    errno = 0;
    ssize_t n =
      send(proc->processor_socket, buffer, buffer_size, MSG_DONTROUTE);
    if (-1 == n) {
      // if no buffer space, just retry
      if (ENOBUFS == errno) {
        pthread_yield();
        continue;
      }
      // if interrupted system call, just retry
      if (EINTR != errno) {
        fprintf(stderr, "reply: send error: %s\n", strerror(errno));
        abort();
        return errno; // negative value
      }
      continue;
    }
    return n;
  }
}

// must use constant string and it includes '\0'
#define const_reply(proc, s)                                                   \
  do {                                                                         \
    ssize_t n = reply(proc, s, sizeof(s));                                     \
    assert(sizeof(s) == n);                                                    \
  } while (0)

// busy device as string
static const char *busy_device(elliott803_t *proc) {
  switch (proc->io_busy) {
  case busy_none:
    return "";
  case busy_reader_1:
    return "reader_1";
  case busy_reader_2:
    return "reader_2";
  case busy_reader_3:
    return "reader_3";
  case busy_punch_1:
    return "punch_1";
  case busy_punch_2:
    return "punch_2";
  case busy_punch_3:
    return "punch_3";
  }
  return "";
}

// action routines
// ---------------

static bool action_terminate(elliott803_t *proc, const char *params) {
  return false;
}

static void system_reset(elliott803_t *proc) {
  proc->program_counter = 0;
  proc->accumulator = 0;
  proc->auxiliary_register = 0;
  proc->overflow = false;
  proc->io_busy = busy_none;
  proc->mode = exec_mode_stop;
  proc->wg_polls = 0;
}

static bool action_reset(elliott803_t *proc, const char *params) {

  system_reset(proc);

  if (0 == strncmp("run", params, 3)) {
    proc->mode = exec_mode_run;
    proc->word_generator = sign_bit; // set as 40  0 : 00  0
    const_reply(proc, "rs run");
  } else {
    proc->mode = exec_mode_stop;
    const_reply(proc, "rs stop");
  }

  return true;
}

static bool action_run(elliott803_t *proc, const char *params) {

  if (exec_mode_run == proc->mode) {
    const_reply(proc, "error already running");
    return true;
  }

  int addr = 0;
  int half = 0;
  for (;;) {
    char c = *params++;
    if (c >= '0' && c <= '9') {
      addr = addr * 10 + c - '0';
      continue;
    } else if ('\0' == c) {
      break;
    } else if ('.' == c) {
      c = *params++;
      if ('0' == c) {
        half = 0;
      } else if ('5' == c) {
        half = 1;
      } else {
        const_reply(proc, "error invalid half bit");
      }
      c = *params++;
      if ('\0' == c) {
        break;
      }
      const_reply(proc, "error invalid address");
    }
    const_reply(proc, "error invalid address");
    return true;
  }
  if (addr >= memory_size) {
    const_reply(proc, "error address too large");
    return true;
  }
  // remember LSB of PC is the half word bit
  proc->program_counter = (addr << 1) | (half & 1);

  // 40 addr : XX    X  or  44 addr : XX    X
  proc->word_generator &= ~(op_a_bits | b_mod_bit);
  proc->word_generator |= ((0 == (half & 1)) ? 040LL : 044LL) << first_op_shift;
  proc->word_generator |= (uint64_t)addr << first_address_shift;
  proc->wg_polls = 0;

  // start running
  proc->mode = exec_mode_run;

  char buffer[256];
  ssize_t n = snprintf(buffer,
                       sizeof(buffer),
                       "run %4d.%d",
                       (proc->program_counter >> 1),
                       5 * (proc->program_counter & 1));
  n = reply(proc, buffer, n + 1); // include '\0'
  assert(0 != n);

  return true;
}

static bool action_cont(elliott803_t *proc, const char *params) {
  proc->mode = exec_mode_run;
  return true;
}
static bool action_stop(elliott803_t *proc, const char *params) {
  proc->mode = exec_mode_stop;
  return true;
}

static bool action_memory_read(elliott803_t *proc, const char *params) {
  int64_t addr = 0;
  for (;;) {
    char c = *params++;
    if (c >= '0' && c <= '9') {
      addr = addr * 10 + c - '0';
    } else if ('\0' == c) {
      break;
    } else {
      const_reply(proc, "error invalid address");
      return true;
    }
  }
  if (addr >= memory_size) {
    const_reply(proc, "error address too large");
    return true;
  }

  int64_t w = core_read_program(proc, addr & address_bits);

  char buffer[256];
  snprintf(buffer, sizeof(buffer), "mr %4" PRId64 ": ", addr);
  char *s = to_machine_code(buffer, w);

  ssize_t n = reply(proc, s, strlen(s) + 1); // include '\0'
  assert(0 != n);
  free(s);

  return true;
}

static bool action_memory_write(elliott803_t *proc, const char *params) {

  int64_t addr = 0;
  for (;;) {
    char c = *params++;
    if (c >= '0' && c <= '9') {
      addr = addr * 10 + c - '0';
    } else if (' ' == c) {
      break;
    } else if ('\0' == c) {
      const_reply(proc, "error missing machine code");
      return true;
    } else {
      const_reply(proc, "error invalid address");
      return true;
    }
  }
  if (addr >= memory_size) {
    const_reply(proc, "error address too large");
    return true;
  }

  int64_t w = from_machine_code(params, strlen(params));
  if (-1LL == w) {
    const_reply(proc, "error invalid machine code");
    return true;
  }
  proc->core_store[addr & address_bits] = w;

  char buffer[256];
  snprintf(buffer, sizeof(buffer), "mw %4" PRId64 ": ", addr);
  char *s = to_machine_code(buffer, w);

  ssize_t n = reply(proc, s, strlen(s) + 1); // include '\0'
  assert(0 != n);
  free(s);

  return true;
}

// print the current machine state
static bool action_current_status(elliott803_t *proc, const char *params) {

  // location and state
  const char *state = "stopped";
  if (exec_mode_run == proc->mode) {
    state = "running";
  }
  int address_p = proc->program_counter >> 1;

  char buffer[256];
  ssize_t n = snprintf(buffer,
                       sizeof(buffer),
                       "sr  SCR:    %4d.%d  [%8s]  [%8s]  [%8s]",
                       address_p,
                       5 * (proc->program_counter & 1),
                       state,
                       proc->overflow ? "overflow" : "",
                       busy_device(proc));
  n = reply(proc, buffer, n + 1); // include '\0'
  assert(0 != n);

  // accumulator
  char *s = to_machine_code("sa  ACC: ", proc->accumulator);
  n = reply(proc, s, strlen(s) + 1); // include '\0'
  assert(0 != n);
  free(s);

  // auxiliary register
  s = to_machine_code("sx  AUX: ", proc->auxiliary_register);
  n = reply(proc, s, strlen(s) + 1); // include '\0'
  assert(0 != n);
  free(s);

  // current location
  int64_t word_p = core_read_program(proc, address_p);
  snprintf(buffer, sizeof(buffer), "sp %4d: ", address_p);
  s = to_machine_code(buffer, word_p);
  n = reply(proc, s, strlen(s) + 1); // include '\0'
  assert(0 != n);
  free(s);

  // address 1 content
  int address_1 = address_bits & (word_p >> first_address_shift);
  int64_t word_1 = core_read(proc, address_1);
  snprintf(buffer, sizeof(buffer), "s1 %4d: ", address_1);
  s = to_machine_code(buffer, word_1);
  n = reply(proc, s, strlen(s) + 1); // include '\0'
  assert(0 != n);
  free(s);

  // address 2 content
  bool b_modified = 0 != (word_p & b_mod_bit);
  if (b_modified) {
    word_p += word_1;
    word_p &= op_b_bits;
    s = to_machine_code("sb BMOD: ", word_p);
    n = reply(proc, s, strlen(s) + 1); // include '\0'
    assert(0 != n);
    free(s);
  }
  int address_2 = address_bits & (word_p >> second_address_shift);
  int64_t word_2 = core_read_program(proc, address_2);
  snprintf(buffer, sizeof(buffer), "s2 %4d: ", address_2);
  s = to_machine_code(buffer, word_2);
  n = reply(proc, s, strlen(s) + 1); // include '\0'
  assert(0 != n);
  free(s);

  return true;
}

// store data into reader buffer
static bool action_reader(elliott803_t *proc, const char *params) {

  int64_t unit = 0;
  for (;;) {
    char c = *params++;
    if (c >= '0' && c <= '9') {
      unit = unit * 10 + c - '0';
    } else if (' ' == c) {
      break;
    } else if ('\0' == c) {
      const_reply(proc, "error missing data bytes");
      return true;
    } else {
      const_reply(proc, "error invalid unit number");
      return true;
    }
  }

  if (unit < 1 || unit > reader_units) {
    const_reply(proc, "error invalid reader unit number");
    return true;
  }

  // must have 1..32 bytes (2..64 chars)
  size_t l = strlen(params);
  if (l < 2 || (1 == (l & 1)) || l > 64) {
    const_reply(proc, "error invalid data length");
    return true;
  }

  l /= 2; // 1..32
  uint8_t bytes[32];
  for (size_t i = 0; i < l; ++i) {
    uint8_t b = 0;
    for (int j = 0; j < 2; ++j) {
      char c = *params++;
      if (c >= '0' && c <= '9') {
        b = (uint8_t)((b << 4) + c - '0');
      } else if (c >= 'a' && c <= 'f') {
        b = (uint8_t)((b << 4) + c - 'a' + 10);
      } else if (c >= 'A' && c <= 'F') {
        b = (uint8_t)((b << 4) + c - 'A' + 10);
      } else {
        const_reply(proc, "error invalid hex digit");
        return true;
      }
    }
    bytes[i] = b;
  }

  // only transfer if all bytes are valid
  for (size_t i = 0; i < l; ++i) {
    buffer_put(&proc->reader[unit - 1], bytes[i]);
  }

  const_reply(proc, "ok");
  return true;
}

// msb/lsb simulate action of Elliott 803 word generator in that
// a single bit can be set, but clear zeros a whole row
// msb    = 40  0 : 00  0   sign bit
// f2l    = 00  0 : 01  0   low bit of op2
// lsb    = 00  0 : 00  1   low bit of address 2
static bool action_word_generator(elliott803_t *proc, const char *params) {

  if ('\0' != params[0]) {
    uint64_t w = proc->word_generator;
    if (0 == strcmp(params, "b")) {
      // set the B-Modifier
      w |= b_mod_bit;
    } else if ('f' == params[0] && ('1' == params[1] || '2' == params[1])) {
      uint64_t shift = '1' == params[1] ? first_op_shift : second_op_shift;
      uint64_t value = 0;
      params += 2;
      for (;;) {
        char c = *params++;
        if (c >= '0' && c <= '7') {
          value = value * 8 + c - '0';
        } else if (' ' == c) {
          continue;
        } else if ('\0' == c) {
          break;
        } else {
          const_reply(proc, "error invalid octal value");
          return true;
        }
      }
      if (value > op_bits) {
        const_reply(proc, "error octal value too large");
        return true;
      }
      if (0 == value) {
        w &= ~(op_bits << shift);
      } else {
        w |= value << shift;
      }
    } else if ('n' == params[0] && ('1' == params[1] || '2' == params[1])) {
      uint64_t shift =
        '1' == params[1] ? first_address_shift : second_address_shift;
      uint64_t value = 0;
      bool bmod = false;
      params += 2;
      for (;;) {
        char c = *params++;
        if (c >= '0' && c <= '9') {
          value = value * 10 + c - '0';
        } else if ('b' == c || '/' == c) {
          if (first_address_shift == shift) {
            bmod = true;
          } else {
            const_reply(proc, "error invalid b-modifier");
            return true;
          }
        } else if (' ' == c) {
          continue;
        } else if ('\0' == c) {
          break;
        } else {
          const_reply(proc, "error invalid decimal value");
          return true;
        }
      }
      if (value >= memory_size) {
        const_reply(proc, "error decimal value too large");
        return true;
      }
      if (0 == value && !bmod) {
        w &= ~(address_bits << shift);
        if (first_address_shift == shift) {
          w &= ~b_mod_bit;
        }
      } else {
        w |= value << shift;
        if (bmod) {
          w |= b_mod_bit;
        }
      }
    } else {
      // otherwise treat as code or ±N
      w = from_machine_code(params, strlen(params));
      if (-1 == (int64_t)(w)) {
        const_reply(proc, "error invalid machine code");
        return true;
      }
    }
    proc->word_generator = w;
  }

  char *s = to_machine_code("wg ", proc->word_generator);

  ssize_t n = reply(proc, s, strlen(s) + 1); // include '\0'
  assert(0 != n);
  free(s);

  return true;
}

// check for stopped or word generator polling
static bool action_check(elliott803_t *proc, const char *params) {

  if (exec_mode_stop == proc->mode) {
    const_reply(proc, "check stop");
    return true;
  }
  int polls = proc->wg_polls;
  proc->wg_polls = 0;

  if (0 == polls) {
    const_reply(proc, "check run");
  } else {
    char buffer[256];
    ssize_t n = snprintf(buffer, sizeof(buffer), "check wg=%d", polls);
    n = reply(proc, buffer, n + 1); // include '\0'
    assert(0 != n);
  }
  return true;
}

static bool action_help(elliott803_t *proc, const char *params) {

  // clang-format: off
  static const char *help[] = {
    "?? reset [run]           reset CPU and optionally run T1 loader",   //
    "?? mw ADDRESS CODE|±N    store code or signed number to address",   //
    "?? mr ADDRESS            display code and numeric for address",     //
    "?? status                display registers and flags",              //
    "?? run ADDRESS[.5]       run from specific address",                //
    "?? cont                  continue after stop",                      //
    "?? stop                  halt the CPU",                             //
    "?? reader UNIT HEX       buffer up to 32 bytes for a reader",       //
    "?? wg                    displays word generator value",            //
    "?? wg CODE|±N            set word generator code or signed number", //
    "?? wg f1|f2 [F]          clear/or wg function 1/2 bits (octal)",    //
    "?? wg n1 [N][/]          clear/or wg address 1 + B bits (decimal)", //
    "?? wg n2 [N]             clear/or wg address 2 bits (decimal)",     //
    "?? check                 check for stop or word generator polling", //
    "?? ",                                                               //
  };
  // clang-format: on
  for (size_t i = 0; i < SizeOfArray(help); ++i) {
    const_reply(proc, help[i]);
  }
  return true;
}

// list of available commands
typedef struct {
  const char *name;
  bool (*fn)(elliott803_t *proc, const char *params);
} cmd_t;

// clang-format: off
static const cmd_t command_list[] = {
  {"reset", action_reset},           //
  {"mw", action_memory_write},       //
  {"mr", action_memory_read},        //
  {"status", action_current_status}, //
  {"run", action_run},               //
  {"cont", action_cont},             //
  {"stop", action_stop},             //
  {"reader", action_reader},         //
  {"wg", action_word_generator},     //
  {"check", action_check},           //
  {"?", action_help},                //
  {"terminate", action_terminate},   // last item (for internal use)
};
// clang-format: on

// main processor control loop
static void *main_loop(void *arg) {

  processor_t *proc = (processor_t *)arg;

  system_reset(proc);

  for (bool run = true; run;) {
    struct timeval tzero = {
      .tv_sec = 0,
      .tv_usec = 0,
    };

    // if running poll for a command
    if (exec_mode_run == proc->mode && busy_none == proc->io_busy) {
      cpu803_execute(proc);
    } else {
      tzero.tv_sec = 1;
    }

    for (size_t i = 0; i < punch_units; ++i) {
      uint8_t b = 0;
      if (buffer_get(&proc->punch[i], &b)) {
        char buffer[256];
        ssize_t n = snprintf(buffer, sizeof(buffer), "p%zu %02x", i + 1, b);
        n = reply(proc, buffer, n + 1); // include '\0'
        assert(0 != n);
      }
    }

    switch (proc->io_busy) {
    case busy_reader_1:
    case busy_reader_2:
    case busy_reader_3: {
      char buffer[256];
      ssize_t n = snprintf(
        buffer, sizeof(buffer), "r%u busy", proc->io_busy - busy_reader_1 + 1);
      n = reply(proc, buffer, n + 1); // include '\0'
      assert(0 != n);
      break;
    }
    default:
      break;
    }

    fd_set in;
    FD_ZERO(&in);
    FD_SET(proc->processor_socket, &in);
    if (select(FD_SETSIZE, &in, NULL, NULL, &tzero) < 0) {
      continue;
    }
    if (!FD_ISSET(proc->processor_socket, &in)) {
      continue;
    }

    // receive a command
    char buffer[message_buffer_size];
    ssize_t n = recv(proc->processor_socket,
                     buffer,
                     sizeof(buffer) - 1,
                     MSG_WAITALL | MSG_DONTWAIT);
    if (n < 0) {
      continue;
    }
    buffer[n] = '\0';

    // locate first space or the '\0'
    char *p = strchrnul(buffer, ' ');
    size_t word_length = p - buffer;

    while (isspace(*p)) {
      ++p;
    }

    // search for and execute command
    for (size_t i = 0; i < SizeOfArray(command_list); ++i) {
      if (0 == strncmp(buffer, command_list[i].name, word_length)) {
        run = command_list[i].fn(proc, p);
        // reset busy to in case some I/O buffer was changed by just
        // executed command; this could be made more specific if
        // command affected the related I/O buffer by providing or
        // consuming data
        if (run) {
          proc->io_busy = busy_none;
        }
        break;
      }
    }
  }
  return NULL;
}

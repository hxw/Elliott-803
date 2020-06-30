// elliott803.h

#if !defined(ELLIOTT803_H)
#define ELLIOTT803_H

#include <sys/types.h>

typedef struct elliott803_struct elliott803_t;

// create a elliott803 instance
// in reset state with zeroed core and registers
elliott803_t *elliott803_create(const char *name);

// destroy the instance
void elliott803_destroy(elliott803_t *proc);

// get the send/receive fd for a use in a select
int elliott803_get_fd(elliott803_t *proc);

// send a command
// returns:
//   positive: bytes sent
//   negative: error code
ssize_t
elliott803_send(elliott803_t *proc, const char *buffer, size_t buffer_size);

// receive a response
// returns:
//   positive: bytes received
//   negative: error code
ssize_t
elliott803_receive(elliott803_t *proc, char *buffer, size_t buffer_size);

#endif

#ifndef JNXU_H
#define JNXU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Similar philosphy as interrupt handler, but for JNXU commands. The pc is
// excluded because a JNXU packet will be received over several interrupts.
// The aux_data is a pointer to the data that the handler needs to do its job
// (similarly to interrupts). The byte array contains the data in the packet.
// The handler is responsible for freeing the buffer.
typedef void (*jnxu_handler_t)(void *, uint8_t *, size_t);

void jnxu_register_handler(uint8_t cmd, jnxu_handler_t fn, void *aux_data);

#endif

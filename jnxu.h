#ifndef JNXU_H
#define JNXU_H

#include "bt_ext.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define JNXU_MAX_MESSAGE_LEN     4096

#define JNXU_PREFIX     '&'

#define JNXU_START      'J'
#define JNXU_END        'X'

#define JNXU_PING       'P'
#define JNXU_ECHO       'E'

// Stuffing is used to avoid sending strings like "AT" or "OK", which could be
// interpreted as AT commands or OK responses by the HC-05 module. Those strings
// are replaced by "A&_T" and "O&_K" respectively. The receiver should ignore
// the stuffing character and the underscore. If we wish to send "&_" itself,
// we simply send "&&_" instead, which escapes the first ampersand.
#define JNXU_STUFFING   '_'

// Similar philosphy as interrupt handler, but for JNXU commands. The pc is
// excluded because a JNXU packet will be received over several interrupts.
// The aux_data is a pointer to the data that the handler needs to do its job
// (similarly to interrupts). The byte array contains the data in the packet.
// If necessary, the handler is responsible to copy the contents to a different
// location in memory, as they are ephemeral to this function call.
typedef void (*jnxu_handler_t)(void *aux_data, const uint8_t *message, size_t len);

void jnxu_register_handler(uint8_t cmd, jnxu_handler_t fn, void *aux_data);

bool jnxu_send(uint8_t cmd, const uint8_t *message, int len);

bool jnxu_ping(void);

void jnxu_init(bt_ext_role_t role, const char *mac);

#endif

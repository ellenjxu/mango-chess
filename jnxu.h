#ifndef JNXU_H
#define JNXU_H

/*
 * Module implementing the JNXU protocol, a simple protocol for sending messages
 * over UART. The protocol is designed to be used in a non-blocking manner,
 * alongside bt_ext.
 *
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 */

/*
 * Protocol Description
 *
 * Overview:
 * The protocol is structured around sending message packets and receiving them
 * in the form of handlers for commands. The idea is similar to UDP ports, such
 * that the same Bluetooth connection can be used for sending different types of
 * data simultaneously. There are 255 possible commands (0 through 255,
 * excluding 38 because it is '&' in ASCII) in each direction (i.e. it is
 * possible to decide that when device A sends command 1 to device B, it is
 * different from sending the same command 1 from B to A). Each message can
 * also include a byte array message, and the user must decide how to serialize
 * the data to a byte array. The message size is limited to JNXU_MAX_MESSAGE_LEN
 * bytes.
 *
 * Structure of a packet:
 * All JNXU packets are preceded by the prefix '&'. The start of a message
 * is thus indicated by '&J'. The next byte corresponds to the command id, a
 * number between 0 and 255. After that, the byte array message is sent,
 * starting from arr[0] through the last element in the array. It is possible
 * to send a message with no payload. Finally, the packet end delimiter is sent
 * '&X'. At any point, we can send '&P' to ping, and the other side must
 * respond with '&E' echo as soon as possible (including while sending a packet).
 *
 * Escaping:
 * To send '&' itself, we send '&&' instead to escape.
 *
 * Stuffing:
 * Stuffing is used to avoid sending strings like "AT" or "OK", which could be
 * interpreted as AT commands or OK responses by the HC-05 module. Those strings
 * are replaced by "A&_T" and "O&_K" respectively. The receiver should ignore
 * the stuffing character and the underscore. If we wish to send "&_" itself,
 * we simply send "&&_" instead, which escapes the first ampersand.
 *
 * Example usage:
 *  - call jnxu_init() with the connection information once.
 *  - call jnxu_register_handler() as many times as necessary to register all
 *      commands.
 *  - call jnxu_send() whenever a message must be send.
 *  - one side should regularly call jnxu_ping() as a sanity check that the
 *      connection is alive.
 */

#include "bt_ext.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define JNXU_MAX_MESSAGE_LEN    4096

#define JNXU_PREFIX     '&'

#define JNXU_START      'J'
#define JNXU_END        'X'

#define JNXU_PING       'P'
#define JNXU_ECHO       'E'

#define JNXU_STUFFING   '_'

// Similar philosphy as interrupt handler, but for JNXU commands. The pc is
// excluded because a JNXU packet will be received over several interrupts.
// The aux_data is a pointer to the data that the handler needs to do its job
// (similarly to interrupts). The byte array contains the data in the packet.
// If necessary, the handler is responsible to copy the contents to a different
// location in memory, as they are ephemeral to this function call.
typedef void (*jnxu_handler_t)(void *aux_data, const uint8_t *message, size_t len);

/*
 * `jnxu_register_handler` registers a handler for a given command.
 *
 * NOTE: trying to register command 38 (or '&') will cause an error.
 * 
 * @param cmd       command id to register
 * @param fn        handler function that will be called when this command is
 *                      received.
 * @param aux_data  a void pointer to any data which the client may find useful
 *                      and which will be passed as an argument to the handler
 */
void jnxu_register_handler(uint8_t cmd, jnxu_handler_t fn, void *aux_data);

/*
 * `jnxu_send` sends a message to the other device.
 *
 * NOTE: sending command 38 (or '&') will cause an error.
 *
 * @param cmd       command to send (i.e. the command identifier which must be
 *                      registered with `jnxu_register_handler` on the other
 *                      device)
 * @param message   message to send, in the form of a byte array
 * @param len       length of the message, in bytes
 * @return          `true` if the message was successfully sent, `false`
 *                      otherwise (note that this does not mean that the message
 *                      was actually received by the other device)
 */
bool jnxu_send(uint8_t cmd, const uint8_t *message, int len);

/*
 * `jnxu_ping` sends a ping message to the other device.
 *
 * @return  `true` if the ping was successfully sent, `false` otherwise (note
 *              that this does not mean we got a response, since that must be
 *              checked asynchronously)
 */
bool jnxu_ping(void);

/*
 * `jnxu_init` initializes the JNXU module.
 *
 * @param role  role of the device in the JNXU protocol
 * @param mac   MAC address of the other device (if role is PRIMARY, otherwise
 *                  this parameter is ignored and can be NULL)
 */
void jnxu_init(bt_ext_role_t role, const char *mac);

#endif

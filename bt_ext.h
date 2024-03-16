#ifndef BT_EXT_H
#define BT_EXT_H

/*
 * `bt_ext` is a module for interfacing with the HC-05 Bluetooth module over
 * UART.
 *
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 *
 * To get a connection going, one party must be PRIMARY and the other must be
 * SUBORDINATE. The PRIMARY party must connect to the SUBORDINATE party. It is
 * possible for either party to not be a Mango Pi (i.e. can connect to phones,
 * etc.). If PRIMARY, you must know the MAC address of the SUBORDINATE.
 *
 * Overview of typical usage:
 * The module is designed to be used in a non-blocking manner. The user should
 * first call bt_ext_init only once to set up the UART module send some initial
 * configuration commands to the Bluetooth module, and set up the module itself.
 * Then, the user should call bt_ext_connect to connect to the other device, as
 * many times as necessary until the connection succeeds. At any time, the user
 * can call bt_ext_has_data to check if there is data available to read from the
 * Bluetooth module. If there is, the user can call bt_ext_read to read the data
 * into a buffer. The user can also register triggers to be called when certain
 * bytes are received from the Bluetooth module (such as characters used as
 * flags in a protocol). The user can also call bt_ext_connected to check if the
 * Bluetooth module is connected to a device and, if not, call bt_ext_connect
 * again to try to connect.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BT_EXT_MAX_BYTES_NO_TRIGGER 127

typedef enum {
    BT_EXT_ROLE_SUBORDINATE = 0,    // BT docs: "slave"
    BT_EXT_ROLE_PRIMARY = 1,        // BT docs: "master"
} bt_ext_role_t;

typedef void (*bt_ext_fn_t)(void);

/*
 * `bt_ext_init` initializes the Bluetooth module.
 */
void bt_ext_init(void);

/*
 * `bt_ext_send_cmd` sends an AT command to the Bluetooth module and waits for a
 * response. The response is stored in the `response` buffer. The `len`
 * parameter specifies the size of the `response` buffer.
 *
 * @param str       AT command to send (including the "AT" prefix)
 * @param response  buffer to store the response in
 * @return          `true` if the command was successful, `false` otherwise.
 */
bool bt_ext_send_cmd(const char *str, uint8_t *response, size_t len);

/*
 * `bt_ext_send_raw` sends a raw byte array to the Bluetooth module.
 *
 * @param byte  byte to send
 */
void bt_ext_send_raw_byte(const uint8_t byte);

/*
 * `bt_ext_send_raw_array` sends a raw byte array to the Bluetooth module.
 *
 * @param buf   buffer to send
 * @param len   length of the buffer in bytes
 */
void bt_ext_send_raw_array(const uint8_t *buf, size_t len);

/*
 * `bt_ext_send_raw_str` sends a null-terminated string to the Bluetooth module.
 *
 * @param buf   null-terminated string to send
 */
void bt_ext_send_raw_str(const char *buf);

/*
 * `bt_ext_read` reads data from the Bluetooth module into a buffer. The
 *
 * @param buf   buffer to read data into
 * @param len   size of the `buf` buffer. Includes a null-terminator to the end
 *              of the buffer for convenience. (This does mean that the buffer
 *              is effectively one byte smaller than the `len` parameter, as one
 *              byte is reserverd for it.)
 * @reutrn      the number of bytes read (not including the null-terminator).
 */
int bt_ext_read(uint8_t *buf, size_t len);

/*
 * `bt_ext_connect` connects to a Bluetooth device with the given MAC address.
 *
 * @param role  role of the Bluetooth module in the connection
 * @param mac   string representation of the MAC address of the device to
 *              connect to (ignored and can be NULL if SUBORDINATE). The MAC
 *              address should be in the format "XXXXXXXXXXXX" (a
 *              null-terminated string of 12 hexadecimal characters, without
 *              any punctuation, such as "685E1C4C31FD").
 */
void bt_ext_connect(const bt_ext_role_t role, const char *mac);

/*
 * `bt_ext_has_data` checks whether there is data available to read from the
 * Bluetooth module.
 *
 * @return  `true` if there is data available to read from the Bluetooth module
 *          over UART, `false` otherwise.
 */
bool bt_ext_has_data(void);

/*
 * `bt_ext_connected` checks whether the module is connected to a Bluetooth
 * device.
 * @return  `true` if the Bluetooth module is connected to a device, `false`
 *          otherwise.
 */
bool bt_ext_connected(void);

/*
 * `bt_ext_register_trigger` registers a function to be called when the given
 * byte is received from the Bluetooth module.Asserts that the byte is not
 * already registered.
 *
 * @param byte  byte to register the function for
 * @param fn    function to be called when the byte is received
 */
void bt_ext_register_trigger(uint8_t byte, bt_ext_fn_t fn);

/*
 * `bt_ext_unregister_trigger` unregisters the function to be called when the
 * given byte is received from the Bluetooth module.
 *
 * @param byte  byte to unregister the function for
 */
void bt_ext_unregister_trigger(uint8_t byte);

/*
 * `bt_ext_register_fallback_trigger` registers a function to be called when too
 * many bytes are received from the Bluetooth module and no trigger is found.
 * This avoids the internal ringbuffer overflowing. (This trigger is called
 * after more than BT_EXT_MAX_BYTES_NO_TRIGGER bytes are received and no trigger
 * is found.)
 *
 * @param fn    function to be called when the fallback trigger is triggered
 */
void bt_ext_register_fallback_trigger(bt_ext_fn_t fn);

#endif

#ifndef BT_EXT_H
#define BT_EXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    BT_EXT_ROLE_SUBORDINATE = 0,    // BT docs: "slave"
    BT_EXT_ROLE_PRIMARY = 1,        // BT docs: "master"
} bt_ext_role_t;

typedef void (*bt_ext_fn_t)(void);

/*
 * `bt_ext_init` initializes the Bluetooth module.
 */
void bt_ext_init();

/*
 * `bt_ext_send_cmd` sends an AT command to the Bluetooth module and waits for a
 * response. The response is stored in the `response` buffer. The `len`
 * parameter specifies the size of the `response` buffer.
 *
 * Returns `true` if the command was successful, `false` otherwise.
 */
bool bt_ext_send_cmd(const char *str, uint8_t *response, size_t len);

void bt_ext_send_raw_array(const uint8_t *buf, size_t len);
void bt_ext_send_raw_byte(const uint8_t buf);
void bt_ext_send_raw_str(const char *buf);

/*
 * `bt_ext_read` reads data from the Bluetooth module into the `buf` buffer. The
 * `len` parameter specifies the size of the `buf` buffer. Adds a null-terminator
 * to the end of the buffer for convenience. (This does mean that the buffer is
 * effectively one byte smaller than the `len` parameter, as one byte is
 * reserverd for it.)
 *
 * Returns the number of bytes read (not including the null-terminator).
 */
int bt_ext_read(uint8_t *buf, size_t len);

/*
 * `bt_ext_connect` connects to a Bluetooth device with the given MAC address.
 * The `role` parameter specifies the role of the Bluetooth module in the
 * connection. The `mac` parameter is a string representation of the MAC
 * address of the device to connect to (ignored if SUBORDINATE). The MAC address
 * should be in the format "XXXXXXXXXXXX" (a null-terminated string of 12
 * hexadecimal characters, without punctuation).
 */
void bt_ext_connect(const bt_ext_role_t role, const char *mac);

/*
 * `bt_ext_has_data` returns `true` if there is data available to read from the
 * Bluetooth module over UART, `false` otherwise.
 */
bool bt_ext_has_data();

/*
 * `bt_ext_connected` returns `true` if the Bluetooth module is connected to a
 * device, `false` otherwise.
 */
bool bt_ext_connected();

/*
 * `bt_ext_register_trigger` registers a function to be called when the given
 * byte is received from the Bluetooth module. The `fn` parameter is the
 * function to be called.
 *
 * Asserts that the byte is not already registered.
 */
void bt_ext_register_trigger(uint8_t byte, bt_ext_fn_t fn);

/*
 * `bt_ext_unregister_trigger` unregisters the function to be called when the
 * given byte is received from the Bluetooth module.
 */
void bt_ext_unregister_trigger(uint8_t byte);

/*
 * `bt_ext_register_fallback_trigger` registers a function to be called when too
 * many bytes are received from the Bluetooth module and no trigger is found.
 * This avoids the internal ringbuffer overflowing. (This trigger is called after
 * 128 bytes are received and no trigger is found.)
 */
void bt_ext_register_fallback_trigger(bt_ext_fn_t fn);

#endif

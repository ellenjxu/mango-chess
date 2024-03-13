#ifndef BT_EXT_H
#define BT_EXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    BT_EXT_ROLE_SUBORDINATE = 0,    // BT docs: "slave"
    BT_EXT_ROLE_PRIMARY = 1,        // BT docs: "master"
} bt_ext_role_t;

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
 * `len` parameter specifies the size of the `buf` buffer.
 *
 * Returns the number of bytes read.
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

#endif

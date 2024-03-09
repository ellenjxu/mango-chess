#ifndef BT_EXT_H
#define BT_EXT_H

#include <stdbool.h>
#include <stddef.h>

/*
 * `bt_ext_init` initializes the Bluetooth module.
 */
void bt_ext_init();

void bt_ext_send(const char *str);

int bt_ext_read(char *buf, size_t len);

bool bt_has_data();

#endif

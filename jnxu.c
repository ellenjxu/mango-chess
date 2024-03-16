/*
 * This module implements a simple protocol for sending messages over Bluetooth.
 * The protocol is designed to be simple and robust. See header file for a
 * detailed description of the protocol.
 *
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 */
#include "assert.h"
#include "strings.h"
#include "jnxu.h"
#include "bt_ext.h"
#include "timer.h"

// Javier Garcia Nieto and Ellen Xu
// ^             ^               ^^    = JNXU

#define NUM_CMDS        256

#define RECONNECT_TIMEOUT_MS 200
#define RECONNECT_RETRIES 5

enum message_state {
    WAITING_FOR_START = 0,
    READING_COMMAND,
    IN_MESSAGE,
};

static struct {
    struct {
        jnxu_handler_t fn;
        void *aux_data;
    } handlers[NUM_CMDS];

    volatile enum message_state state;
    volatile bool saw_prefix;

    uint8_t cmd;
    uint8_t message[JNXU_MAX_MESSAGE_LEN];
    int message_len;

    bt_ext_role_t role;
    char mac[13];

    unsigned long last_ping;
    unsigned long last_echo;
} module;

void jnxu_register_handler(uint8_t cmd, jnxu_handler_t fn, void *aux_data) {
    module.handlers[cmd].fn = fn;
    module.handlers[cmd].aux_data = aux_data;
}

static bool ensure_connected(void) {
    for (int i = 0; i < RECONNECT_RETRIES; i++) {
        if (bt_ext_connected()) {
            return true;
        }

        bt_ext_connect(module.role, module.mac);
        timer_delay_ms(RECONNECT_TIMEOUT_MS);
    }

    return false;
}

bool jnxu_send(uint8_t cmd, const uint8_t *message, int len) {
    if (!ensure_connected()) {
        return false;
    }

    bt_ext_send_raw_byte(JNXU_PREFIX);
    bt_ext_send_raw_byte(JNXU_START);
    bt_ext_send_raw_byte(cmd);
    for (int i = 0; i < len; i++) {
        switch (message[i]) {
            case JNXU_PREFIX:
                bt_ext_send_raw_byte(JNXU_PREFIX);
                break;
            case 'T':
                if (i > 0 && message[i - 1] == 'A') {
                    bt_ext_send_raw_byte(JNXU_PREFIX);
                    bt_ext_send_raw_byte(JNXU_STUFFING);
                }
                break;
            case 'K':
                if (i > 0 && message[i - 1] == 'O') {
                    bt_ext_send_raw_byte(JNXU_PREFIX);
                    bt_ext_send_raw_byte(JNXU_STUFFING);
                }
                break;
        }
        bt_ext_send_raw_byte(message[i]);
    }
    bt_ext_send_raw_byte(JNXU_PREFIX);
    bt_ext_send_raw_byte(JNXU_END);

    return true;
}

bool jnxu_ping(void) {
    if (!ensure_connected()) {
        return false;
    }

    module.last_ping = timer_get_ticks();
    bt_ext_send_raw_byte(JNXU_PREFIX);
    bt_ext_send_raw_byte(JNXU_PING);

    return true;
}

static void process_byte(uint8_t byte) {
    if (module.saw_prefix) {
        module.saw_prefix = false;
        switch (byte) {
            case JNXU_START:
                module.state = READING_COMMAND;
                break;
            case JNXU_END:
                if (module.state != IN_MESSAGE) {
                    module.state = WAITING_FOR_START;
                    break;
                }
                module.state = WAITING_FOR_START;
                if (module.handlers[module.cmd].fn != NULL) {
                    module.handlers[module.cmd].fn(module.handlers[module.cmd].aux_data, module.message, module.message_len);
                }
                break;
            case JNXU_PING:
                bt_ext_send_raw_byte(JNXU_PREFIX);
                bt_ext_send_raw_byte(JNXU_ECHO);
                break;
            case JNXU_ECHO:
                module.last_echo = timer_get_ticks();
                break;
            case JNXU_STUFFING:
                // do nothing (see above for explanation)
                break;
            case JNXU_PREFIX:
                // escape sequence for &
                goto normal_process;
                break;
            default:
                module.state = WAITING_FOR_START;
        }
    } else if (byte == JNXU_PREFIX) {
        module.saw_prefix = true;
        if (module.state == READING_COMMAND)
            module.state = WAITING_FOR_START;
    } else if (module.state == READING_COMMAND) {
        module.cmd = byte;
        module.state = IN_MESSAGE;
        module.message_len = 0;
    } else if (module.state == IN_MESSAGE) {
normal_process:
        assert(module.message_len < sizeof(module.message));
        module.message[module.message_len++] = byte;
    } // else ignore byte
}

static void process_uart(void) {
    uint8_t buf[1024];
    while (bt_ext_has_data()) {
        int len = bt_ext_read(buf, sizeof(buf));
        for (int i = 0; i < len; i++) {
            process_byte(buf[i]);
        }
    }
}

void jnxu_init(bt_ext_role_t role, const char *mac) {
    module.role = role;
    memcpy(module.mac, mac, sizeof(module.mac));

    bt_ext_init();
    ensure_connected();

    bt_ext_register_trigger(JNXU_PREFIX, process_uart);
    bt_ext_register_trigger(JNXU_START, process_uart);
    bt_ext_register_trigger(JNXU_END, process_uart);
    bt_ext_register_trigger(JNXU_PING, process_uart);
    bt_ext_register_trigger(JNXU_ECHO, process_uart);
}


/*
 * Module implementing the JNXU protocol, a simple protocol for sending messages
 * over UART. The protocol is designed to be used in a non-blocking manner,
 * alongside bt_ext, but can be easily modified to use a similar module for
 * a different communication medium.
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

#define RECONNECT_DELAY_USEC (5 * 1000 * 1000) // 5 seconds
#define RECONNECT_CHECKS 10
#define RECONNECT_RETRIES 1

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

    volatile unsigned long last_ping;
    volatile unsigned long last_echo;
} module;

void jnxu_register_handler(uint8_t cmd, jnxu_handler_t fn, void *aux_data) {
    assert(cmd != JNXU_PREFIX);
    module.handlers[cmd].fn = fn;
    module.handlers[cmd].aux_data = aux_data;
}

/*
 * This function ensures that the device is connected to the other end of the
 * Bluetooth link. It will attempt to reconnect if the device is not connected.
 *
 * @return  `true` if the device is connected, `false` otherwise.
 */
static bool ensure_connected(void) {
    // TODO: set module to not connected if we have missed an echo
    for (int i = 0; i < RECONNECT_RETRIES; i++) {
        if (bt_ext_connected()) {
            return true;
        }

        bt_ext_connect(module.role, module.mac);

        // give time to connect, but leave if we are connected earlier
        for (int i = 0; i < RECONNECT_CHECKS; i++) {
            if (bt_ext_connected()) {
                return true;
            }

            timer_delay_us(RECONNECT_DELAY_USEC / RECONNECT_CHECKS);
        }
    }

    return false;
}

bool jnxu_send(uint8_t cmd, const uint8_t *message, int len) {
    assert(cmd != JNXU_PREFIX);

    if (!ensure_connected()) {
        return false;
    }

    // start of message
    bt_ext_send_raw_byte(JNXU_PREFIX);
    bt_ext_send_raw_byte(JNXU_START);

    // send the command id
    bt_ext_send_raw_byte(cmd);

    // send each byte of the message, escaping as necessary
    for (int i = 0; i < len; i++) {
        switch (message[i]) {
            case JNXU_PREFIX:
                // when sending a prefix, we need to escape it with another
                // prefix (i.e. send && instead of &)
                bt_ext_send_raw_byte(JNXU_PREFIX);
                break;
            case 'T':
                // avoid sending "AT", which would be interpreted as a command
                // by the HC-05 module, so send "A&_T" instead
                if (i > 0 && message[i - 1] == 'A') {
                    bt_ext_send_raw_byte(JNXU_PREFIX);
                    bt_ext_send_raw_byte(JNXU_STUFFING);
                }
                break;
            case 'K':
                // avoid sending "OK", which would be interpreted as a response
                // by the HC-05 module, so send "O&_K" instead
                if (i > 0 && message[i - 1] == 'O') {
                    bt_ext_send_raw_byte(JNXU_PREFIX);
                    bt_ext_send_raw_byte(JNXU_STUFFING);
                }
                break;
        }
        // after escaping, send the byte
        bt_ext_send_raw_byte(message[i]);
    }

    // end of message
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

/*
 * This function processes a byte received from the Bluetooth module. It
 * processes the byte according to the current state of the protocol.
 *
 * @param byte  byte to process.
 */
static void process_byte(uint8_t byte) {
    if (module.saw_prefix) {
        // previous character was prefix

        // clear the flag
        module.saw_prefix = false;

        // explore which character we are dealing with
        switch (byte) {
            case JNXU_START:
                // start of message
                module.state = READING_COMMAND;
                break;
            case JNXU_END:
                // end of message
                if (module.state != IN_MESSAGE) {
                    // not in message, ignore
                    module.state = WAITING_FOR_START;
                    break;
                }

                // was in message, call the appropriate handler
                module.state = WAITING_FOR_START;
                if (module.handlers[module.cmd].fn != NULL) {
                    module.handlers[module.cmd].fn(module.handlers[module.cmd].aux_data, module.message, module.message_len);
                }

                break;
            case JNXU_PING:
                // respond to ping with echo
                bt_ext_send_raw_byte(JNXU_PREFIX);
                bt_ext_send_raw_byte(JNXU_ECHO);
                break;
            case JNXU_ECHO:
                // update last echo time
                module.last_echo = timer_get_ticks();
                break;
            case JNXU_STUFFING:
                // do nothing (see header file or jnxu_send for explanation)
                break;
            case JNXU_PREFIX:
                // escape sequence for prefix (i.e. send && instead of &)
                goto normal_process;
                break;
            default:
                // unknown character
                module.state = WAITING_FOR_START;
        }
    } else if (byte == JNXU_PREFIX) {
        // saw prefix, write down for the next iteration
        module.saw_prefix = true;
    } else if (module.state == READING_COMMAND) {
        // this byte is the command
        module.cmd = byte;
        module.state = IN_MESSAGE;
        module.message_len = 0;
    } else if (module.state == IN_MESSAGE) {
        // normal character, add to message
normal_process:
        assert(module.message_len < sizeof(module.message));
        module.message[module.message_len++] = byte;
    } // else ignore byte
}

/*
 * This function processes the incoming data from the Bluetooth module. It
 * clears the queue byte by byte, calling `process_byte` for each byte.
 */
static void process_uart(void) {
    // use twice the theoretical maximum length to avoid buffer overflows
    uint8_t buf[2 * BT_EXT_MAX_BYTES_NO_TRIGGER];
    while (bt_ext_has_data()) {
        int len = bt_ext_read(buf, sizeof(buf));
        for (int i = 0; i < len; i++) {
            process_byte(buf[i]);
        }
    }
}

void jnxu_init(bt_ext_role_t role, const char *mac) {
    // copy the role and mac address
    module.role = role;
    memcpy(module.mac, mac, sizeof(module.mac));

    // initialize the connection
    bt_ext_init();
    ensure_connected();

    // register trigers for all relevant characters
    bt_ext_register_trigger(JNXU_PREFIX, process_uart);
    bt_ext_register_trigger(JNXU_START, process_uart);
    bt_ext_register_trigger(JNXU_END, process_uart);
    bt_ext_register_trigger(JNXU_PING, process_uart);
    bt_ext_register_trigger(JNXU_ECHO, process_uart);
}


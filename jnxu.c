#include <assert.h>
#include "jnxu.h"
#include "bt_ext.h"
#include <stdbool.h>
#include <stdint.h>

// Javier Nieto and Ellen Xu
// ^      ^               ^^    = JNXU

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

#define NUM_CMDS        256

#define MESSAGE_LEN     4096

enum message_state {
    WAITING_FOR_START,
    READING_COMMAND,
    IN_MESSAGE,
};

static struct {
    struct {
        jnxu_handler_t fn;
        void *aux_data;
    } handlers[NUM_CMDS];
    enum message_state state;
    bool saw_prefix;

    uint8_t cmd;
    uint8_t message[MESSAGE_LEN];
    int message_len;
} module;

void jnxu_register_handler(uint8_t cmd, jnxu_handler_t fn, void *aux_data) {
    module.handlers[cmd].fn = fn;
    module.handlers[cmd].aux_data = aux_data;
}

bool jnxu_send(uint8_t cmd, const uint8_t *message, int len) {
    if (!bt_ext_connected()) {
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

static void process_byte(uint8_t byte) {
    if (module.saw_prefix) {
        module.saw_prefix = false;
        switch (byte) {
            case JNXU_START:
                module.state = READING_COMMAND;
                break;
            case JNXU_END:
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
                // TODO
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
    } else if (byte == '&') {
        module.saw_prefix = true;
    } else if (module.state == READING_COMMAND) {
        module.cmd = byte;
        module.state = IN_MESSAGE;
    } else if (module.state == IN_MESSAGE) {
normal_process:
        assert(module.message_len < sizeof(module.message));
        module.message[module.message_len++] = byte;
    } // else ignore byte
}


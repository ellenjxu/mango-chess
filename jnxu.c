#include <assert.h>
#include "jnxu.h"
#include <stdbool.h>
#include <stdint.h>

// Javier Nieto and Ellen Xu
// ^      ^               ^^    = JNXU

#define JNXU_PREFIX     '&'

#define JNXU_START      'J'
#define JNXU_END        'X'

#define JNXU_PING       'P'
#define JNXU_ECHO       'E'

#define NUM_CMDS        256

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
    uint8_t message[2048];
    int message_len;
} module;

void jnxu_register_handler(uint8_t cmd, jnxu_handler_t fn, void *aux_data) {
    module.handlers[cmd].fn = fn;
    module.handlers[cmd].aux_data = aux_data;
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
                // TODO
                break;
            case JNXU_ECHO:
                // TODO
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

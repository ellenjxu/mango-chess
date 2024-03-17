#include "chess_commands.h"
#include "interrupts.h"
#include "jnxu.h"
#include "printf.h"
#include "timer.h"
#include "uart.h"
#include "chess.h"
#include "chess_gui.h"
#include <stddef.h>
#include <stdint.h>

#define BT_MODE BT_EXT_ROLE_SUBORDINATE
#define BT_MAC  NULL

typedef enum {
    LISTENING_X0,
    LISTENING_Y0,
    LISTENING_X1,
    LISTENING_Y1,
    LISTENING_PROMOTION,
} brain_state_t;

static struct {
    volatile int cursor_x;
    volatile int cursor_y;

    volatile brain_state_t state;
} module;

typedef struct {
    brain_state_t state;
    int x;
} brain_update_t;

static void update_cursor(void *aux_data, const uint8_t *message, size_t len) {
    if (len < 1) return;

    int position = message[0];
    bool is_piece_taken = true;

    switch (module.state) {
        case LISTENING_X0:
            is_piece_taken = false;
        case LISTENING_X1:
            module.cursor_x = position;
            break;
        case LISTENING_Y0:
            is_piece_taken = false;
        case LISTENING_Y1:
            module.cursor_y = position;
            break;
        default:
            break;
    }
    // draw cursor immediately
    chess_gui_draw_cursor(module.cursor_x, module.cursor_y, is_piece_taken);
}

static void button_press(void *aux_data, const uint8_t *message, size_t len) {
    if (len < 1) return;

    int position = message[0];
    // send info to stockfish

    // add (brain-state, position) to ringbuffer
}

int main(void) {
    interrupts_init();
    interrupts_global_enable();
    uart_init();

    chess_gui_init();

    chess_gui_print();
    timer_delay(5);

    jnxu_init(BT_MODE, BT_MAC);

    jnxu_register_handler(CMD_CURSOR, update_cursor, NULL);
    jnxu_register_handler(CMD_PRESS, button_press, NULL);
    // chess_gui_update("e8g8\n");

    while (1) {
        // at some point:
        // jnxu_send(CMD_MOVE, POINTER TO STOCKFISH COMMAND, LENGTH);
    }
}

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

#define BLACK -1
#define WHITE 1

#define PLAYING BLACK

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
    char move[6];
} module;

static const char promotion_piece_names[] = {'', 'q', 'r', 'b', 'n'};

static void update_cursor(void *aux_data, const uint8_t *message, size_t len) {
    if (len < 1) return;

    int position = message[0];
    bool is_piece_moved = true;

    switch (module.state) {
        case LISTENING_X0:
            is_piece_moved = false;
        case LISTENING_X1:
            module.cursor_x = position;
            break;
        case LISTENING_Y0:
            is_piece_moved = false;
        case LISTENING_Y1:
            module.cursor_y = position;
            break;
        default:
            break;
    }

    chess_gui_draw_cursor(module.cursor_x, module.cursor_y, is_piece_moved);
}

static void button_press(void *aux_data, const uint8_t *message, size_t len) {
    if (len < 1) return;

    int position = message[0];
    module.move[module.state] = position; // store move
    
    if (module.state == LISTENING_PROMOTION) {  // end of move
        char move[6];
        move[0] = 'a' + module.move[0];
        move[2] = 'a' + module.move[2];
        move[4] = promotion_piece_names[module.move[4]];
        if (PLAYING == BLACK) {
            move[1] = '8' - module.move[1];
            move[3] = '8' - module.move[3];
        } else {
            move[1] = '1' + module.move[1];
            move[3] = '1' + module.move[3];
        }
        move[5] = '\n';
        chess_send_move(move); // send move to stockfish
        chess_gui_update(move);
        char *new_move = chess_get_move(); // get stockfish move
        chess_gui_update(new_move);
        jnxu_send(CMD_MOVE, new_move, 6); // send stockfish move to hand
    }

    module.state = (module.state + 1) % 5; // next state
}

static void reset_move(void *aux_data, const uint8_t *message, size_t len) {
    module.state = LISTENING_X0;
    module.cursor_x = 0;
    module.cursor_y = 0;
    chess_gui_draw_cursor(module.cursor_x, module.cursor_y, false);
    // shouldn't need to reset module.move because it will be overwritten
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
    jnxu_register_handler(CMD_RESET_MOVE, reset_move, NULL);

    while (1) {
        // at some point:
        // jnxu_send(CMD_MOVE, POINTER TO STOCKFISH COMMAND, LENGTH);
    }
}

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
    char move[6];
} module;

static const char promotion_piece_names[] = {'q', 'r', 'b', 'n'};

static void update_cursor(void *aux_data, const uint8_t *message, size_t len) {
    if (len < 1) return;

    int position = message[0];

    switch (module.state) {
        case LISTENING_X0:
            module.cursor_x = position;
            break;
        case LISTENING_X1:
            module.cursor_x += position;
            break;
        case LISTENING_Y0:
            module.cursor_y = position;
            break;
        case LISTENING_Y1:
            module.cursor_y += position;
            break;
        case LISTENING_PROMOTION:
            module.cursor_x = position;
            module.cursor_y = 0;
            break;
    }

#if PLAYING == WHITE
    int visual_cursor_x = module.cursor_x;
    int visual_cursor_y = module.cursor_y;
#else
    int visual_cursor_x = CHESS_SIZE - module.cursor_x - 1;
    int visual_cursor_y = CHESS_SIZE - module.cursor_y - 1;
#endif

    bool is_piece_moved = module.state == LISTENING_X1 || module.state == LISTENING_Y1;

    chess_gui_draw_cursor(visual_cursor_x, visual_cursor_y, is_piece_moved);
}

static void button_press(void *aux_data, const uint8_t *message, size_t len) {
    if (len < 1) return;

    // uart_putstring("button press\n");

    int position = message[0];
    module.move[module.state] = position; // store move
    
    if (module.state == LISTENING_PROMOTION) {  // end of move
        char move[6];
        move[0] = 'a' + module.move[0];
        move[2] = 'a' + module.move[2];
        
        if (PLAYING == BLACK) {
            move[1] = '8' - module.move[1];
            move[3] = '8' - module.move[3];
        } else {
            move[1] = '1' + module.move[1];
            move[3] = '1' + module.move[3];
        }

        if (position) { // if position = 0, no promotion. TODO: do we want to double press?
            move[4] = promotion_piece_names[position - 1];
            move[5] = '\n';
        } else {
            move[4] = '\n';
        }
        chess_send_move(move); // send move to stockfish
        chess_gui_update(move);
        char *new_move = chess_get_move(); // get stockfish move
        chess_gui_update(new_move);
        jnxu_send(CMD_MOVE, (const uint8_t *)new_move, 6); // send stockfish move to hand
    }

    module.state = (module.state + 1) % 5; // next state
}

static void reset_move(void *aux_data, const uint8_t *message, size_t len) {
    module.state = LISTENING_X0;
    module.cursor_x = 0;
    module.cursor_y = 0;
    chess_gui_draw_cursor(module.cursor_x, module.cursor_y, false);
}

int main(void) {
    interrupts_init();
    interrupts_global_enable();
    uart_init();

    chess_gui_init();

    // chess_gui_print();

    jnxu_init(BT_MODE, BT_MAC);

    jnxu_register_handler(CMD_CURSOR, update_cursor, NULL);
    jnxu_register_handler(CMD_PRESS, button_press, NULL);
    jnxu_register_handler(CMD_RESET_MOVE, reset_move, NULL);

    // test
    update_cursor(NULL, (const uint8_t[]){3}, 1);
    // chess_gui_draw_cursor(0, 0, false);
    // timer_delay(5);
    // chess_gui_draw_cursor(1, 1, false);
    // chess_gui_draw_cursor(1, 2, true);

    while (1) {
        // at some point:
        // jnxu_send(CMD_MOVE, POINTER TO STOCKFISH COMMAND, LENGTH);
    }
}

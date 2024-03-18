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
#define CLAMP(x, min, max) ((x) > (max) ? (max) : ((x) < (min) ? (min) : (x)))

typedef enum {
    LISTENING_X0 = 0,
    LISTENING_Y0,
    LISTENING_X1,
    LISTENING_Y1,
    LISTENING_PROMOTION,
} brain_state_t;

static struct {
    volatile int cursor_x;
    volatile int cursor_y;
    volatile int cursor_promotion;
    volatile brain_state_t state;
    int move[5];
} module;

static const char promotion_piece_names[] = { 'r', 'n', 'b', 'q', 'q', 'b', 'n', 'r' };

static void update_cursor(void *aux_data, const uint8_t *message, size_t len) {
    if (len < 1) return;

    int motion = (message[0] == MOTION_CW) ? 1 : -1;
    uart_putchar(message[0]);

    switch (module.state) {
        case LISTENING_X0:
        case LISTENING_X1:
            module.cursor_x += motion;
            break;
        case LISTENING_Y0:
        case LISTENING_Y1:
            module.cursor_y += motion;
            break;
        case LISTENING_PROMOTION:
            // TODO fancy popup promotion
            module.cursor_promotion += motion;
            break;
    }

    module.cursor_x = CLAMP(module.cursor_x, 0, 7);
    module.cursor_y = CLAMP(module.cursor_y, 0, 7);
    module.cursor_promotion = CLAMP(module.cursor_promotion, -1, 7);

    bool is_piece_moved = module.state == LISTENING_X1 || module.state == LISTENING_Y1;
    int visual_cursor_x, visual_cursor_y;

    if (module.state == LISTENING_PROMOTION) {
        visual_cursor_x = module.cursor_promotion;
        visual_cursor_y = 0;
    } else {
#if PLAYING == WHITE
        visual_cursor_x = module.cursor_x;
        visual_cursor_y = module.cursor_y;
#else
        visual_cursor_x = CHESS_SIZE - module.cursor_x - 1;
        visual_cursor_y = CHESS_SIZE - module.cursor_y - 1;
#endif
    }

    chess_gui_draw_cursor(visual_cursor_x, visual_cursor_y, is_piece_moved);
}

static void button_press(void *aux_data, const uint8_t *message, size_t len) {
    switch (module.state) {
        case LISTENING_X0:
        case LISTENING_X1:
            module.move[module.state] = module.cursor_x;
            break;
        
        case LISTENING_Y1:
            module.cursor_promotion = -1;
        case LISTENING_Y0:
            module.move[module.state] = module.cursor_x;
            break;

        case LISTENING_PROMOTION:
            {
                char move[6];

#if PLAYING == BLACK
                move[0] = 'h' - module.move[0];
                move[1] = '8' - module.move[1];
                move[2] = 'h' - module.move[2];
                move[3] = '8' - module.move[3];
#else
                move[0] = 'a' + module.move[0];
                move[1] = '1' + module.move[1];
                move[2] = 'a' + module.move[2];
                move[3] = '1' + module.move[3];
#endif

                if (module.cursor_promotion >= 0) { // if position = 0, no promotion. TODO: do we want to double press?
                    move[4] = promotion_piece_names[module.cursor_promotion];
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
            break;
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

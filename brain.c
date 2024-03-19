#include "chess_commands.h"
#include "interrupts.h"
#include "jnxu.h"
#include "printf.h"
#include "timer.h"
#include "uart.h"
#include "chess.h"
#include "chess_gui.h"
#include "strings.h"
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

static void reset_cursor(void) {
    module.cursor_x = 0;
    module.cursor_y = CHESS_SIZE - 1;
    module.cursor_promotion = 0;
}

static void paint_cursor(void) {
    bool is_piece_moved = module.state == LISTENING_X1 || module.state == LISTENING_Y1;

#if PLAYING == WHITE
    int visual_cursor_x = module.cursor_x;
    int visual_cursor_y = module.cursor_y;
#else
    int visual_cursor_x = CHESS_SIZE - module.cursor_x - 1;
    int visual_cursor_y = CHESS_SIZE - module.cursor_y - 1;
#endif
    chess_gui_draw_cursor(visual_cursor_x, visual_cursor_y, is_piece_moved);
}

static void update_cursor(void *aux_data, const uint8_t *message, size_t len) {
    if (len < 1) return;

    int motion = (message[0] == MOTION_CW) ? 1 : -1;

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

    if (module.state == LISTENING_PROMOTION) {
        int visual_cursor_x = module.cursor_promotion;
        int visual_cursor_y = 0;
        chess_gui_draw_cursor(visual_cursor_x, visual_cursor_y, false);
    } else {
        paint_cursor();
    }
}

static void button_press(void *aux_data, const uint8_t *message, size_t len) {
    switch (module.state) {
        case LISTENING_X1:
        case LISTENING_X0:
            module.move[module.state] = module.cursor_x;
            break;
        
        case LISTENING_Y1:
            module.cursor_promotion = -1;
        case LISTENING_Y0:
            module.move[module.state] = module.cursor_y;
            break;

        case LISTENING_PROMOTION:
            {
                char opp_move[6];

#if PLAYING == BLACK
                opp_move[0] = 'h' - module.move[0];
                opp_move[1] = '8' - module.move[1];
                opp_move[2] = 'h' - module.move[2];
                opp_move[3] = '8' - module.move[3];
#else
                opp_move[0] = 'a' + module.move[0];
                opp_move[1] = '1' + module.move[1];
                opp_move[2] = 'a' + module.move[2];
                opp_move[3] = '1' + module.move[3];
#endif

                if (module.cursor_promotion >= 0) {
                    opp_move[4] = promotion_piece_names[module.cursor_promotion];
                    opp_move[5] = '\n';
                } else {
                    opp_move[4] = '\n';
                }

                chess_send_move(opp_move); // send move to stockfish

                char *your_move = chess_get_move(); // get stockfish move 
                if (strcmp(your_move, "NOPE\n") != 0) {
                    chess_gui_update(opp_move);
                    chess_gui_update(your_move);
                    jnxu_send(CMD_MOVE, (const uint8_t *)your_move, 6); // send stockfish move to hand
                }

                reset_cursor();
                paint_cursor();
            }
            break;
    }

    module.state = (module.state + 1) % 5; // next state

    if (module.state == LISTENING_X1) { // show selected on button bress
        paint_cursor();
    }
}

static void reset_move(void *aux_data, const uint8_t *message, size_t len) {
    module.state = LISTENING_X0;
    reset_cursor();
    chess_gui_draw_cursor(module.cursor_x, module.cursor_y, false);
}

int main(void) {
    interrupts_init();
    interrupts_global_enable();
    uart_init();

    jnxu_init(BT_MODE, BT_MAC);
    jnxu_register_handler(CMD_CURSOR, update_cursor, NULL);
    jnxu_register_handler(CMD_PRESS, button_press, NULL);
    jnxu_register_handler(CMD_RESET_MOVE, reset_move, NULL);

    chess_gui_init();
    reset_cursor();
    paint_cursor();
    chess_init();
    // chess_gui_print();

#if PLAYING == WHITE
    char *your_move = chess_get_move();
    chess_gui_update(your_move);
    jnxu_send(CMD_MOVE, (const uint8_t *)your_move, 6);
#endif

    while (1) {
    }
}

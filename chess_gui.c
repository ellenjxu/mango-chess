/*
 * Module for chess GUI. Displays a chess board on the screen. Includes
 * function to update UI based on new move.
 *
 * Author: Ellen Xu <ellenjxu@stanford.edu>
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 */
#include "chess_gui.h"
#include "printf.h"
#include "uart.h"
#include "gl.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "gpio_interrupt.h"
#include "timer.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define CHESS_BLACK gl_color(124, 149, 93)
#define CHESS_WHITE gl_color(238, 238, 213)

// initialize
static int board[CHESS_SIZE][CHESS_SIZE] = {
    {BR, BN, BB, BQ, BK, BB, BN, BR}
    {BP, BP, BP, BP, BP, BP, BP, BP},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {WR, WN, WB, WQ, WK, WB, WN, WR},
    {WP, WP, WP, WP, WP, WP, WP, WP},
};

void chess_gui_draw(void) {
    // draw the chess board
    int s = SCREEN_HEIGHT/8 > SCREEN_WIDTH/8 ? SCREEN_WIDTH/8 : SCREEN_HEIGHT/8;

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if ((i + j) % 2 == 0) {
                gl_draw_rect(s*i, s*j, s, s, CHESS_WHITE);
            }
            else {
                gl_draw_rect(s*i, s*j, s, s, CHESS_BLACK);
            }
            // draw the piece name
            switch (board[j][i]) {
                case WP:
                    gl_draw_char(s*i, s*j, 'P', GL_WHITE);
                    break;
                case WN:
                    gl_draw_char(s*i, s*j, 'N', GL_WHITE);
                    break;
                case WB:
                    gl_draw_char(s*i, s*j, 'B', GL_WHITE);
                    break;
                case WR:
                    gl_draw_char(s*i, s*j, 'R', GL_WHITE);
                    break;
                case WQ:
                    gl_draw_char(s*i, s*j, 'Q', GL_WHITE);
                    break;
                case WK:
                    gl_draw_char(s*i, s*j, 'K', GL_WHITE);
                    break;
                case BP:
                    gl_draw_char(s*i, s*j, 'p', GL_BLACK);
                    break;
                case BN:
                    gl_draw_char(s*i, s*j, 'n', GL_BLACK);
                    break;
                case BB:
                    gl_draw_char(s*i, s*j, 'b', GL_BLACK);
                    break;
                case BR:
                    gl_draw_char(s*i, s*j, 'r', GL_BLACK);
                    break;
                case BQ:
                    gl_draw_char(s*i, s*j, 'q', GL_BLACK);
                    break;
                case BK:
                    gl_draw_char(s*i, s*j, 'k', GL_BLACK);
                    break;
                default:
                    break;
            }
        }
    }
}

void chess_gui_update(const char* move) {
    // UCI format: e2e4\n
    int x1 = move[0] - 'a';
    int y1 = move[1] - '1';
    int x2 = move[2] - 'a';
    int y2 = move[3] - '1';

    board[y2][x2] = board[y1][x1];
    board[y1][x1] = XX;

    chess_gui_draw();
}

void chess_gui_print(void) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            printf("%d ", board[i][j]);
        }
        printf("\n");
    }
}

void chess_gui_init(void) {

    gl_init(SCREEN_WIDTH, SCREEN_HEIGHT, GL_DOUBLEBUFFER);

    uart_putstring("Clearing screen\n");
    gl_clear(GL_WHITE); 
    gl_swap_buffer();

    uart_putstring("Done\n");

    chess_gui_draw();
    gl_swap_buffer();

    while (1) {

    }
}

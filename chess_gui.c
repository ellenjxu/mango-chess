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

// Options: CHESS_COM_GREEN, CHESS_COM_BLUE, MANGO
#define THEME MANGO

#if THEME == MANGO
#define CHESS_BLACK gl_color(188, 81, 150)
#define CHESS_WHITE gl_color(243, 216, 95)
#elif THEME == CHESS_COM_BLUE
#define CHESS_BLACK gl_color( 84, 114, 150)
#define CHESS_WHITE gl_color(234, 233, 212)
#else
#define CHESS_BLACK gl_color(124, 149, 93)
#define CHESS_WHITE gl_color(238, 238, 213)
#endif

// initialize
static int board[CHESS_SIZE][CHESS_SIZE] = {
    {WR, WN, WB, WQ, WK, WB, WN, WR},
    {WP, WP, WP, WP, WP, WP, WP, WP},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {BP, BP, BP, BP, BP, BP, BP, BP},
    {BR, BN, BB, BQ, BK, BB, BN, BR}
};

void chess_gui_draw(int b[CHESS_SIZE][CHESS_SIZE]) {
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
        }
    }
}

void chess_gui_update(const char* move) {
    // UCI format: e2e4\n
    int x1 = move[0] - 'a';
    int y1 = move[1] - '1';
    int x2 = move[2] - 'a';
    int y2 = move[3] - '1';

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

    chess_gui_draw(board);
    gl_swap_buffer();

    while (1) {

    }
}

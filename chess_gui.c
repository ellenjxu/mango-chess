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
    printf("hi");
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if ((i + j) % 2 == 0) {
                gl_draw_rect(50*i, 50*j, 50, 50, GL_GREEN);
            }
            else {
                gl_draw_rect(50*i, 50 * j, 50, 50, GL_YELLOW);
            }
        }
    }

    // TODO: print the piece name using board array
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

    const int WIDTH = 300;
    const int HEIGHT = 300;

    gl_init(WIDTH, HEIGHT, GL_DOUBLEBUFFER);

    uart_putstring("Clearing screen\n");
    gl_clear(GL_WHITE); 
    gl_swap_buffer();
    gl_draw_rect(0, 0, 50, 50, GL_GREEN);
    gl_swap_buffer();

    uart_putstring("Done\n");

    // chess_gui_draw(board);
}

/* File: Chess GUI
 * -------------
 * Displays the current chess game. Includes function to update UI based on new move.
 */
#include "chess_gui.h"
#include "printf.h"
#include "uart.h"
#include "gl.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "gpio_interrupt.h"
#include "timer.h"

void draw_board(void) {
    // draw the chess board
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if ((i + j) % 2 == 0) {
                gl_draw_rect(100 * i, 100 * j, 100, 100, GL_GREEN);
            }
            else {
                gl_draw_rect(100 * i, 100 * j, 100, 100, GL_YELLOW);
            }
        }
    }

    // draw the pieces (for now just write the letter)
    for (int i = 0; i < 8; i++) {
        gl_draw_char(100 * i + 50, 100, 'P', GL_BLACK);
        gl_draw_char(100 * i + 50, 600, 'P', GL_WHITE);
    }
    for (int i = 0; i < 8; i++) {
        char piece;
        switch (i) {
            case 0:
            case 7:
                piece = 'R';
                break;
            case 1:
            case 6:
                piece = 'N';
                break;
            case 2:
            case 5:
                piece = 'B';
                break;
            case 3:
                piece = 'Q';
                break;
            case 4:
                piece = 'K';
                break;
        }
        gl_draw_char(100 * i + 50, 700, piece, GL_WHITE);
        gl_draw_char(100 * i + 50, 200, piece, GL_BLACK);
    }
}

void update_board(const char* move) {
    // UCI format: e2e4\n
    int x1 = move[0] - 'a';
    int y1 = move[1] - '1';
    int x2 = move[2] - 'a';
    int y2 = move[3] - '1';

}

void main(void) {
    gpio_init();
    timer_init();
    uart_init();
    interrupts_init();

    const int WIDTH = 800;
    const int HEIGHT = 1600;

    gl_init(WIDTH, HEIGHT, GL_SINGLEBUFFER);
    gl_clear(GL_WHITE); 

    draw_board();
}

/*
 * Module for chess GUI. Displays a chess board on the screen. Includes
 * function to update UI based on new move.
 *
 * Author: Ellen Xu <ellenjxu@stanford.edu>
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 */
#include "chess_gui.h"
#include "gl.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "gpio_interrupt.h"
#include "printf.h"
#include "strings.h"
#include "timer.h"
#include "uart.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define SQUARE_SIZE (SCREEN_HEIGHT > SCREEN_WIDTH ? SCREEN_WIDTH/8 : SCREEN_HEIGHT/8)
#define PADDING 2

#define MANGO           0
#define CHESS_COM_GREEN 1
#define CHESS_COM_BLUE  2

#define THEME MANGO

#define SHOW_LETTERS true
#define SHOW_NUMBERS true

#define PIECE_BLACK GL_BLACK
#define PIECE_WHITE GL_WHITE

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

static const char chess_gui_piece_names[] = { ' ', 'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k' };

// initialize
static const chess_gui_piece_t STARTING_BOARD[CHESS_SIZE][CHESS_SIZE] = {
    {BR, BN, BB, BQ, BK, BB, BN, BR},
    {BP, BP, BP, BP, BP, BP, BP, BP},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {XX, XX, XX, XX, XX, XX, XX, XX},
    {WP, WP, WP, WP, WP, WP, WP, WP},
    {WR, WN, WB, WQ, WK, WB, WN, WR},
};

static chess_gui_piece_t board[CHESS_SIZE][CHESS_SIZE];

static chess_gui_piece_t taken[4 * CHESS_SIZE];
static int taken_count = 0;

static bool is_white(chess_gui_piece_t piece) {
    switch (piece) {
        case WP:
        case WN:
        case WB:
        case WR:
        case WQ:
        case WK:
            return true;
        default:
            return false;
    }
}

void chess_gui_draw(void) {
    // draw the chess board
    for (int row = 0; row < 8; row++) {
        int black_square = row % 2;
        for (int col = 0; col < 8; col++) {
            gl_draw_rect(
                    SQUARE_SIZE*col,
                    SQUARE_SIZE*row,
                    SQUARE_SIZE,
                    SQUARE_SIZE,
                    black_square ? CHESS_BLACK : CHESS_WHITE
                    );

            gl_draw_char(
                    SQUARE_SIZE*col + SQUARE_SIZE / 2 - gl_get_char_width() / 2,
                    SQUARE_SIZE*row + SQUARE_SIZE / 2 - gl_get_char_height() / 2,
                    chess_gui_piece_names[board[row][col]],
                    is_white(board[row][col]) ? PIECE_WHITE : PIECE_BLACK
                    );

            if (row == 7 && SHOW_LETTERS) {
                gl_draw_char(
                        SQUARE_SIZE*(col + 1) - gl_get_char_width()  - PADDING,
                        SQUARE_SIZE*(row + 1) - gl_get_char_height() - PADDING,
                        'a' + col, black_square ? CHESS_WHITE : CHESS_BLACK
                        );
            }

            if (col == 0 && SHOW_NUMBERS) {
                gl_draw_char(
                        SQUARE_SIZE*col + PADDING,
                        SQUARE_SIZE*row + PADDING,
                        '1' + 7 - row, black_square ? CHESS_WHITE : CHESS_BLACK
                        );
            }

            black_square = !black_square;
        }
    }
    gl_swap_buffer();
}

void chess_gui_update(const char *move) {
    // UCI format: e2e4\n
    int col1 = move[0] - 'a';
    int row1 = CHESS_SIZE - (move[1] - '1') - 1;
    int col2 = move[2] - 'a';
    int row2 = CHESS_SIZE - (move[3] - '1') - 1;

    if (board[row2][col2] != XX) {
        taken[taken_count++] = board[row2][col2];
    }

    // castling
    if (board[row1][col1] == WK && col1 == 4 && row1 == 7) {
        if (col2 == 6) {
            board[7][7] = XX;
            board[7][5] = WR;
        } else if (col2 == 2) {
            board[7][0] = XX;
            board[7][3] = WR;
        }
    } else if (board[row1][col1] == BK && col1 == 4 && row1 == 0) {
        if (col2 == 6) {
            board[0][7] = XX;
            board[0][5] = BR;
        } else if (col2 == 2) {
            board[0][0] = XX;
            board[0][3] = BR;
        }
    }

    board[row2][col2] = board[row1][col1];
    board[row1][col1] = XX;

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
    memcpy(board, STARTING_BOARD, sizeof(STARTING_BOARD));
    chess_gui_draw();
}

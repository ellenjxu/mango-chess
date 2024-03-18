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
#include <stdint.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define SQUARE_SIZE (SCREEN_HEIGHT > SCREEN_WIDTH ? SCREEN_WIDTH/8 : SCREEN_HEIGHT/8)
#define PADDING 2

#define MANGO           0
#define CHESS_COM_GREEN 1
#define CHESS_COM_BLUE  2

#define THEME CHESS_COM_BLUE

#define SHOW_LETTERS true
#define SHOW_NUMBERS true

#define PIECE_BLACK GL_BLACK
#define PIECE_WHITE GL_WHITE

#define THIN_CURSOR     3
#define THICK_CURSOR    5

#if THEME == MANGO
#define CHESS_BLACK gl_color(188,  81, 150)
#define CHESS_WHITE gl_color(243, 216, 95)
#elif THEME == CHESS_COM_BLUE
#define CHESS_BLACK gl_color( 84, 114, 150)
#define CHESS_WHITE gl_color(234, 233, 212)
#else
#define CHESS_BLACK gl_color(124, 149, 93)
#define CHESS_WHITE gl_color(238, 238, 213)
#endif

#define CURSOR_COLOR gl_color(222, 187, 11) // #debb0b lol

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

static uint8_t stale[CHESS_SIZE][CHESS_SIZE];

static struct {
    int chosen_row;
    int chosen_col;
    bool has_chosen;

    int row;
    int col;
} cursor;

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

/*
* Draws a border around a rectangle.
*/
static void draw_border(int x, int y, int width, int height, int thickness, color_t color) {
    gl_draw_rect(x, y, width, thickness, color);
    gl_draw_rect(x, y, thickness, height, color);
    gl_draw_rect(x + width - thickness, y, thickness, height, color);
    gl_draw_rect(x, y + height - thickness, width, thickness, color);
}

static void stale_everything(void) {
    memset(stale, 1, sizeof(stale));
}

void chess_gui_draw(void) {
    // draw the chess board
    for (int row = 0; row < 8; row++) {
        int black_square = row % 2;
        for (int col = 0; col < 8; col++) {
            if (stale[row][col]) {
                gl_draw_rect(
                        SQUARE_SIZE*col,
                        SQUARE_SIZE*row,
                        SQUARE_SIZE,
                        SQUARE_SIZE,
                        black_square ? CHESS_BLACK : CHESS_WHITE
                        );

                if (row == cursor.row && col == cursor.col && cursor.has_chosen) {
                    gl_draw_char(
                            SQUARE_SIZE*col + SQUARE_SIZE / 2 - gl_get_char_width() / 2,
                            SQUARE_SIZE*row + SQUARE_SIZE / 2 - gl_get_char_height() / 2,
                            chess_gui_piece_names[board[cursor.chosen_row][cursor.chosen_col]],
                            CURSOR_COLOR
                            );
                } else if (row == cursor.chosen_row && col == cursor.chosen_col && cursor.has_chosen) {
                    // draw nothing
                } else {
                    gl_draw_char(
                            SQUARE_SIZE*col + SQUARE_SIZE / 2 - gl_get_char_width() / 2,
                            SQUARE_SIZE*row + SQUARE_SIZE / 2 - gl_get_char_height() / 2,
                            chess_gui_piece_names[board[row][col]],
                            is_white(board[row][col]) ? PIECE_WHITE : PIECE_BLACK
                            );
                }

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

                if (row == cursor.row && col == cursor.col) {
                    draw_border(
                            row * SQUARE_SIZE,
                            col * SQUARE_SIZE,
                            SQUARE_SIZE,
                            SQUARE_SIZE,
                            cursor.has_chosen ? THICK_CURSOR : THIN_CURSOR,
                            CURSOR_COLOR
                            );
                }
            }

            black_square = !black_square;
            stale[row][col] = 0;
        }
    }
}

void chess_gui_draw_cursor(int cursor_col, int cursor_row, bool is_piece_chosen) {
    if (is_piece_chosen && !cursor.has_chosen) {
        cursor.chosen_col = cursor.col;
        cursor.chosen_row = cursor.row;
    }

    stale[cursor.row][cursor.col] = 1;

    printf("0 cursor_row: %d, cursor_col: %d\n", cursor_row, cursor_col);

    cursor.has_chosen = is_piece_chosen;

    cursor.col = cursor_col;
    cursor.row = CHESS_SIZE - cursor_row - 1;

    stale[cursor.row][cursor.col] = 1;

    printf("1 cursor_row: %d, cursor_col: %d\n", cursor_row, cursor_col);
    chess_gui_draw();
}

void chess_gui_update(const char *move) {
    // UCI format: e2e4\n

    int col1 = move[0] - 'a';
    int col2 = move[2] - 'a';

    // row 1 in chess notation is at the bottom of our GUI, so we must invert it
    int row1 = CHESS_SIZE - (move[1] - '1') - 1;
    int row2 = CHESS_SIZE - (move[3] - '1') - 1;

    // keep track of taken pieces
    if (board[row2][col2] != XX) {
        taken[taken_count++] = board[row2][col2];
    }

    // castling
    if (board[row1][col1] == WK && col1 == 4 && row1 == 7) {
        if (col2 == 6) {
            // white short castle
            board[7][7] = XX;
            board[7][5] = WR;
        } else if (col2 == 2) {
            // white long castle
            board[7][0] = XX;
            board[7][3] = WR;
        }
    } else if (board[row1][col1] == BK && col1 == 4 && row1 == 0) {
        if (col2 == 6) {
            // black short castle
            board[0][7] = XX;
            board[0][5] = BR;
        } else if (col2 == 2) {
            // black long castle
            board[0][0] = XX;
            board[0][3] = BR;
        }
    }

    switch (move[4]) {
        case 'R':
        case 'r':
            // rook promotion
            board[row2][col2] = is_white(board[row1][col1]) ? WR : BR;
            break;

        case 'Q':
        case 'q':
            // queen promotion
            board[row2][col2] = is_white(board[row1][col1]) ? WQ : BQ;
            break;

        case 'B':
        case 'b':
            // bishop promotion
            board[row2][col2] = is_white(board[row1][col1]) ? WB : BB;
            break;

        case 'N':
        case 'n':
            // knight promotion
            board[row2][col2] = is_white(board[row1][col1]) ? WN : BN;
            break;

        default:
            // no promotion, use piece in previous position
            board[row2][col2] = board[row1][col1];
            break;
    }

    board[row1][col1] = XX;

    stale_everything();
    chess_gui_draw();
}

void chess_gui_print(void) {
    printf("\n+---+---+---+---+---+---+---+---+\n");
    for (int i = 0; i < 8; i++) {
        printf("|");
        for (int j = 0; j < 8; j++) {
            printf(" %c |", chess_gui_piece_names[board[i][j]]);
        }
        printf("\n+---+---+---+---+---+---+---+---+\n");
    }
}

void chess_gui_init(void) {
    gl_init(SCREEN_WIDTH, SCREEN_HEIGHT, GL_SINGLEBUFFER);
    memcpy(board, STARTING_BOARD, sizeof(STARTING_BOARD));
    cursor.col = -1;
    cursor.row = -1;
    stale_everything();
    chess_gui_draw();
}

#ifndef CHESS_GUI_H
#define CHESS_GUI_H

#ifndef CHESS_SIZE
#define CHESS_SIZE 8
#endif
/*
 * Module for chess GUI. Displays a chess board on the screen. Includes
 * function to update UI based on new move.
 *
 * Author: Ellen Xu <ellenjxu@stanford.edu>
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 */

#include <stdbool.h>

typedef enum {
    XX = 0, // empty square
    WP,     // white pawn
    WN,     // white knight
    WB,     // white bishop
    WR,     // white rook
    WQ,     // white queen
    WK,     // white king
    BP,     // black pawn
    BN,     // black knight
    BB,     // black bishop
    BR,     // black rook
    BQ,     // black queen
    BK      // black king
} chess_gui_piece_t;


/*
 * `chess_gui_draw` draws the chess board on the screen.
 */
void chess_gui_draw(void);

/*
 * `chess_gui_draw_cursor` draws a border for cursor at the given position.
 *
 * @param x                 the x position of the cursor
 * @param y                 the y position of the cursor
 * @param is_piece_moved    whether the piece has been moved
 */
void chess_gui_draw_cursor(int x, int y, bool is_piece_moved);

/*
 * `chess_gui_update` updates the UI based on the new move.
 *
 * @param move  the move to be updated, with UCI format (e.g. "e2e4\n",
 *              "e7e8q\n" for promotion, etc.). Only the first 5 characters are
 *              accessed.
 */
void chess_gui_update(const char* move);

/*
 * `chess_gui_print` prints the current board state to the console.
 */
void chess_gui_print(void);

/*
 * `chess_gui_init` initializes the chess module and the graphics library with
 * gl_init()
 *
 * Calling it multiple times will reset the chessboard.
 */
void chess_gui_init(void);

#endif

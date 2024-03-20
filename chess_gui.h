#ifndef CHESS_GUI_H
#define CHESS_GUI_H

#ifndef CHESS_SIZE
#define CHESS_SIZE 8
#endif

/*
 * Module for chess GUI. Displays a chess board on the screen. Includes
 * function to update UI based on new move.
 *
 * For efficiency, the module does its best to only redraw parts of the
 * chessboard that are stale by keeping track of updates. It also uses
 * DOUBLEBUFFER to avoid artifacts on the screen.
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
 * @param move      the move to be updated, with UCI format (e.g. "e2e4\n",
 *                  "e7e8q\n" for promotion, etc.). Only the first 5 characters
 *                  are accessed.
 * @param engine    whether the move was performed by the engine (to highlight)
 */
void chess_gui_update(const char *move, bool engine);

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

/*
 * `chess_gui_stats` prints the win, draw, and loss statistics to the screen.
 *
 * If a pointer is NULL, the corresponding statistic is not updated.
 *
 * @param W pointer to char representing the percentage of wins in the form of
 *              "42" for 42% or "3" for 3%, or NULL if not to be updated.
 * @param D similar to W, but for draws.
 * @param L similar to W, but for losses.
 */
void chess_gui_stats(char *W, char *D, char *L);

/*
 * `chess_gui_promote` draws the promotion menu on the screen.
 * 
 * @param cursor    the cursor position (0-3) to highlight the selected piece
 */
void chess_gui_promote(int cursor);

/*
 * `chess_gui_sidebar` draws the sidebar on the screen.
 */
void chess_gui_sidebar(void);

#endif

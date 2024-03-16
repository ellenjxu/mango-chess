#ifndef CHESS_GUI_H
#define CHESS_GUI_H

/*
 * Module for chess GUI. Displays a chess board on the screen. Includes
 * function to update UI based on new move.
 *
 * Author: Ellen Xu <ellenjxu@stanford.edu>
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 */

#ifndef CHESS_SIZE
#define CHESS_SIZE 8
#endif

enum { XX = 0, WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK };

void chess_gui_draw(void);
void chess_gui_update(const char* move);
void chess_gui_print(void);
void chess_gui_init(void);

#endif

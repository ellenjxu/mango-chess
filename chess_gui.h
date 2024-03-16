#ifndef CHESS_GUI_H
#define CHESS_GUI_H

/*
 * Module for chess GUI
 *
 * Author: Ellen Xu <ellenjxu@stanford.edu>
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 */

#include <stddef.h>
#define N 8

enum { XX = 0, WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK };

void draw_board(int b[N][N]);
void update_board(const char* move);
void print_board(void);
void board_init(void);

#endif
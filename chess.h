#ifndef CHESS_H
#define CHESS_H

/*
 * Module for Stockfish 
 *
 * Author: Ellen Xu <ellenjxu@stanford.edu>
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 */

char* chess_predict();
void chess_send_move(const char* move);
void chess_init(void);

#endif
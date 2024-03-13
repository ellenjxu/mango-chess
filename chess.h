#ifndef CHESS_H
#define CHESS_H

/*
 * Module for Stockfish 
 *
 * Author: Ellen Xu <ellenjxu@stanford.edu>
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 */

char* read_move();
void send_move(const char* move);
void chess_game(void);

#endif
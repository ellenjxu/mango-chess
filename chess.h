#ifndef CHESS_H
#define CHESS_H

/*
 * Module for Stockfish. Communicates over UART with Python script running
 * Stockfish on a powerful machine (the Mango Pi would struggle).
 *
 * Author: Ellen Xu <ellenjxu@stanford.edu>
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 */

/*
 * `chess_get_move` returns the move that the Stockfish engine has calculated.
 */
char *chess_get_move(void);
void chess_send_move(const char* move);
void chess_init(void);

#endif

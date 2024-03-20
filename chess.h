#ifndef CHESS_H
#define CHESS_H

/*
 * Module for Stockfish. Communicates over UART with Python script running
 * Stockfish on a powerful machine (the Mango Pi would struggle).
 *
 * Author: Ellen Xu <ellenjxu@stanford.edu>
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 */

#include <stddef.h>

/*
 * `chess_get_move` gets the move that the Stockfish engine has calculated.
 *
 * May also be used to receive general messages from Stockfish.
 *
 * @param buf   pointer to an array of characters to store the message.
 * @param size  size of buf in bytes.
 *
 * NOTE: function ASSERTS that size >= 8.
 */
void chess_get_move(char buf[], size_t size);

/*
 * `chess_send_move` sends a move to the Stockfish engine.
 *
 * @param move  string representing the move to be sent. Must be in the format
 *              of UCI long algebraic notation, ending with a newline and a null
 *              terminator. For example, e2e4\n\0
 */
void chess_send_move(const char *move);

/*
 * `chess_init` initializes the UART communication with the Stockfish engine.
 */
void chess_init(void);

/*
 * `chess_next_command` returns the next command from the Stockfish engine.
 *
 * @return  pointer to the next command from the Stockfish engine. Caller is
 *          responsible for freeing the memory. NULL if no command available.
 */
char *chess_next_command(void);

#endif

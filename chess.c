/* Module for communicating to Stockfish `engine.py` via UART. */

#include "assert.h"
#include "chess.h"
#include "uart.h"
#include "printf.h"
#include "strings.h"
#include "chess_commands.h"

void chess_get_move(char move[], size_t bufsize) {
    assert(bufsize >= 8);

    /* Gets the move from Stockfish */
    int i = 0;
    char ch;

    do {
        ch = uart_getchar();
        move[i++] = ch;
    } while (ch != '\n' && ch != '\0' && i < 7);

    move[i] = '\0';
}

void chess_send_move(const char* move) {
    /* Sends a move to Stockfish (\n terminated)*/
    uart_putstring("\nMOVE_BEGIN\n");
    uart_putstring(move);
}

void chess_init(void) {
#if PLAYING == WHITE
    uart_putstring("\nGAME_WHITE\n");
#else
    uart_putstring("\nGAME_BLACK\n");
#endif
    char ack[8];

    while (1) {
        chess_get_move(ack, sizeof(ack)); // get the ACK from engine

        if (strcmp(ack, "READY\n") == 0) break;
    }
}

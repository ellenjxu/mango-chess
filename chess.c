/* Module for communicating to Stockfish `engine.py` via UART. */

#include "assert.h"
#include "chess.h"
#include "uart.h"
#include "printf.h"
#include "malloc.h"
#include "strings.h"
#include "ringbuffer_ptr.h"
#include "chess_commands.h"
#include <stdint.h>

static rb_ptr_t *rb;

void chess_get_move(char buf[], size_t bufsize) {
    assert(bufsize >= 8);

    char move[256];

    /* Gets the move from Stockfish */
    int i = 0;
    char ch;

was_command:
    do {
        ch = uart_getchar();
        move[i++] = ch;

        if (i >= 7 && move[0] != '/')
            break;
        else if (i >= sizeof(move) - 1)
            break;

    } while (ch != '\n' && ch != '\0');

    move[i] = '\0';

    if (move[0] == '/') {   // command
        char *cmd = malloc(i);
        memcpy(cmd, move + 1, i);
        rb_ptr_enqueue(rb, (uintptr_t)cmd);
        i = 0;
        goto was_command;
    } else {
        memcpy(buf, move, bufsize);
    }
}

char *chess_next_command(void) {
    uintptr_t ptr = 0;
    rb_ptr_dequeue(rb, &ptr);
    return (char *)ptr;
}

void chess_send_move(const char* move) {
    /* Sends a move to Stockfish (\n terminated)*/
    uart_putstring("\nMOVE_BEGIN\n");
    uart_putstring(move);
}

void chess_init(void) {
    rb = rb_ptr_new();

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

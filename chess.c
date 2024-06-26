/*
 * Module for communicating to Stockfish `engine.py` via UART.
 *
 * See `engine.py` for details on how the protocol works.
 * 
 * Author: Ellen Xu <ellenjxu@stanford.edu>
 */

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

    // When we receive information from the host laptop, there is a chance that
    // it might be a command, in which case we enqueue it and jump back to the
    // top of the function.
    char move[256];

    int i = 0;
    char ch;

was_command:
    do {
        ch = uart_getchar();
        move[i++] = ch;

        if (i >= 7 && move[0] != '/')   // if not command and too long, break
            break;
        else if (i >= sizeof(move) - 1) // if command and too long for command
            break;

    } while (ch != '\n' && ch != '\0');

    move[i] = '\0';

    if (move[0] == '/') {   // found command
        char *cmd = malloc(i);
        memcpy(cmd, move + 1, i);
        rb_ptr_enqueue(rb, (uintptr_t)cmd);

        // caller is expecting a move, so we jump back and see if we get one
        i = 0;
        goto was_command;
    } else {
        // copy the move to buffer
        memcpy(buf, move, bufsize);
    }
}

char *chess_next_command(void) {
    uintptr_t ptr = 0;
    rb_ptr_dequeue(rb, &ptr);
    return (char *)ptr;
}

void chess_send_move(const char* move) {
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

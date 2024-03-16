/* reads from UART to get the next chess move */

#include "chess.h"
#include "uart.h"
#include "malloc.h"
#include "printf.h"
#include "strings.h"

char *chess_get_move(void) {
    char *move = malloc(8 * sizeof(char));

    int i = 0;
    char ch;
    while (1) {
        ch = uart_getchar();
        move[i++] = ch;
        
        if (ch == '\n' || ch == '\0' || i >= 7) {
            move[i] = '\0';
            break;
        }
    }
    return move;
}

void chess_send_move(const char* move) {
    /* Sends a move (\n terminated)*/
    uart_putstring("MOVE_BEGIN\n");
    uart_putstring(move);
}

void chess_init() { // TODO: white or black
    uart_putstring("GAME_BEGIN\n");
    while (true) {
        char* start = chess_get_move();
        if (strcmp(start, "READY\n") == 0) break;
    }
    chess_send_move("e2e4\n");
    chess_get_move();
}

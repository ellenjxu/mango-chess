/* reads from UART to get the next chess move */

#include "chess.h"
#include "uart.h"
#include "malloc.h"
#include "printf.h"
#include "strings.h"

char* read_move(void) {
    char* move = malloc(8 * sizeof(char));

    int i=0;
    char ch;
    while (true) {
        ch = uart_getchar();
        move[i++] = ch;
        
        if (ch == '\n' || ch == '\0' || i >= 7) {
            move[i] = '\0';
            break;
        }
    }
    return move;
}

void send_move(const char* move) {
    uart_putstring(move);
}

void chess_game(void) {
    uart_putstring("GAME_BEGIN\n");
    while (true) {
        char* start = read_move();
        if (strcmp(start, "READY\n") == 0) break;
    }
    send_move("e2e4\n");
    read_move();
}
/* reads from UART to get the next chess move */

#include "chess.h"
#include "uart.h"
#include "malloc.h"
#include "printf.h"
#include "strings.h"

char* chess_predict(void) {
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

void chess_send_move(const char* move) {
    uart_putstring(move);
}

void chess_init(void) {
    uart_putstring("GAME_BEGIN\n");
    while (true) {
        char* start = chess_predict();
        if (strcmp(start, "READY\n") == 0) break;
    }
    chess_send_move("e2e4\n");
    chess_predict();
}
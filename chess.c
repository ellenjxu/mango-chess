/* reads from UART to get the next chess move */

#include "chess.h"
#include "uart.h"
#include "malloc.h"
#include "printf.h"

unsigned char* read_move(void) {
    unsigned char* move = malloc(8 * sizeof(char));

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
    printf("%s", move);
    return move;
}

void send_move(char* move) {
    
}
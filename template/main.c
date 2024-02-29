/* File: main.c
 * -------------
 * Sample main program.
 */
#include "printf.h"
#include "uart.h"

void main(void) {
    uart_init();
    uart_putstring("Hello, world!\n");
    printf("I am printf, here m%c %s!\n", 'e', "ROAR");
}

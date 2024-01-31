#include "printf.h"
#include "timer.h"
#include "uart.h"

void main (void) {
    uart_init();    // must set up uart peripheral before using, init once

    for (int i = 0; i < 5; i++) {
        uart_putstring("hello, laptop\n");
        timer_delay(1);
    }
    printf("We %s printf!\n", "<3");
}

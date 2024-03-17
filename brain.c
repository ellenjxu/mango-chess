#include "bt_ext.h"
#include "gpio.h"
#include "interrupts.h"
#include "jnxu.h"
#include "printf.h"
#include "re.h"
#include "timer.h"
#include "uart.h"
#include <stdint.h>
#include "chess.h"
#include "chess_gui.h"
#include "gl.h"

int main(void) {
    interrupts_init();
    interrupts_global_enable();
    uart_init();

    chess_gui_init();

    chess_gui_print();
    timer_delay(5);
    chess_gui_update("e8g8\n");

    while (1) {}
}

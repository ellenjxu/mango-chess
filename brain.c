#include "interrupts.h"
#include "jnxu.h"
#include "printf.h"
#include "timer.h"
#include "uart.h"
#include "chess.h"
#include "chess_gui.h"
#include <stddef.h>

#define BT_MODE BT_EXT_ROLE_SUBORDINATE
#define BT_MAC  NULL

int main(void) {
    interrupts_init();
    interrupts_global_enable();
    uart_init();

    chess_gui_init();

    chess_gui_print();
    timer_delay(5);

    jnxu_init(BT_MODE, BT_MAC);
    // chess_gui_update("e8g8\n");

    while (1) {}

}

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

void brain(void) {
    interrupts_init();
    interrupts_global_enable();
    uart_init();

    chess_gui_init();

    timer_delay(5);

    chess_gui_update("d2d4\n");
    timer_delay(1);
    chess_gui_update("g1f3\n");
    timer_delay(1);
    chess_gui_update("g2g3\n");
    timer_delay(1);
    chess_gui_update("f1g2\n");
    timer_delay(1);
    chess_gui_update("e1g1\n");
    timer_delay(1);
    // chess_gui_init();


    while (1) {}
}

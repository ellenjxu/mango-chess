#include "bt_ext.h"
#include "gpio.h"
#include "interrupts.h"
#include "jnxu.h"
#include "printf.h"
#include "re.h"
#include "uart.h"
#include <stdint.h>
#include "chess.h"
#include "chess_gui.h"
#include "gl.h"

void brain(void) {
    interrupts_init();
    interrupts_global_enable();
    uart_init();
    // chess_init();
    
    chess_gui_init();

    // while (1) {
    //     char* move = chess_get_move();
    //     chess_gui_update(move);
    //     chess_send_move("e7e5\n");
    // }
}

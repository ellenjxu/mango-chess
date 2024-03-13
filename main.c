#include "gpio.h"
#include "interrupts.h"
#include "uart.h"
#include "re.h"
#include "printf.h"
#include "bt_ext.h"
#include <stdint.h>
#include "chess.h"

#define RE_CLOCK GPIO_PB0
#define RE_DATA GPIO_PD22
#define RE_SW GPIO_PD21 // (button)

#define MGPIA_MAC "685E1C4C31FD"
#define MGPIB_MAC "685E1C4C0016"

static void terminal_bluetooth(void) {
    interrupts_init();
    interrupts_global_enable();
    uart_init();
    bt_ext_init();
    char cmd[1024];
    int cmd_len = 0;
    uint8_t result[1024];

    while (1) {
        if (*result) {
            uart_putstring((char *)result);
            uart_putchar('\n');
            result[0] = '\0';
        }

        if (bt_ext_has_data()) {
            uint8_t buf[1024];
            int len = bt_ext_read(buf, sizeof(buf));
            if (len > 0) {
                uart_putstring((char *)buf);
            }
        }

        if (uart_haschar()) {
            char c = uart_getchar();
            if (c == '\b') {
                if (cmd_len > 0) {
                    cmd_len--;
                    cmd[cmd_len] = '\0';
                    uart_putchar('\b');
                    uart_putchar(' ');
                }
            } else if (c == '\r') {
            } else if (c == '\n') {
                cmd[cmd_len] = '\0';
                cmd_len = 0;
                bt_ext_send_cmd(cmd, result, sizeof(result));
            } else {
                cmd[cmd_len++] = c;
            }
            uart_putchar(c);
        }
    }
}

int main(void) {
    terminal_bluetooth();
    uart_init();
    bt_ext_init();
    // terminal_bluetooth();
    chess_game();
}

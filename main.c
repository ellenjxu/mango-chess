#include "gpio.h"
#include "interrupts.h"
#include "jnxu.h"
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

static void handle_message(void *data, const uint8_t *bytes, size_t len) {
    uart_putstring("Received: ");
    for (int i = 0; i < len; i++) {
        uart_putchar(bytes[i]);
    }
    uart_putchar('\n');
}

static void jnxu_test(void) {
    interrupts_init();
    interrupts_global_enable();
    uart_init();
    jnxu_init(BT_EXT_ROLE_PRIMARY, MGPIB_MAC);
    jnxu_register_handler(1, handle_message, NULL);

    while (1) {
        // spin
    }
}

int main(void) {
    jnxu_test();
    // terminal_bluetooth();
    // chess_game();
}

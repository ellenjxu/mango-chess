#include "gpio.h"
#include "interrupts.h"
#include "uart.h"
#include "re.h"
#include "printf.h"
#include "bt_ext.h"

#define RE_CLOCK GPIO_PB0
#define RE_DATA GPIO_PD22
#define RE_SW GPIO_PD21 // (button)

// int main(void) {
//     uart_init();
//     interrupts_init();
//     re_device_t *dev = re_new(RE_CLOCK, RE_DATA, RE_SW);
//     interrupts_global_enable();
//     
//     while (1) {
//         re_event_t event = re_read_blocking(dev);
//         if (event == RE_EVENT_CLOCKWISE) {
//             printf("clockwise\n");
//         } else if (event == RE_EVENT_COUNTERCLOCKWISE) {
//             printf("counterclockwise\n");
//         } else if (event == RE_EVENT_PUSH) {
//             printf("push\n");
//         }
//     }
// }

int main(void) {
    bt_ext_init();
    gpio_init();
    interrupts_init();
    uart_init();

    interrupts_global_enable();

    char cmd[1024];
    int cmd_len = 0;
    while (1) {
        if (bt_has_data()) {
            char buf[1024];
            int len = bt_ext_read(buf, sizeof(buf));
            if (len > 0) {
                uart_putstring(buf);
            }
        }

        if (uart_haschar()) {
            char c = uart_getchar();
            if (c == '\r') {
            } else if (c == '\n') {
                cmd[cmd_len] = '\0';
                cmd_len = 0;
                bt_ext_send(cmd);
            } else {
                cmd[cmd_len++] = c;
            }
            uart_putchar(c);
        }
    }
}

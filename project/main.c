#include "gpio.h"
#include "uart.h"
#include "re.h"
#include "printf.h"

#define RE_CLOCK GPIO_PB0
#define RE_DATA GPIO_PD22
#define RE_SW GPIO_PD21 // (button)

int main(void) {
    uart_init();
    interrupts_init();
    re_device_t *dev = re_new(RE_CLOCK, RE_DATA, RE_SW);
    interrupts_global_enable();
    
    while (1) {
        re_event_t event = re_read_blocking(dev);
        if (event == RE_EVENT_CLOCKWISE) {
            printf("clockwise\n");
        } else if (event == RE_EVENT_COUNTERCLOCKWISE) {
            printf("counterclockwise\n");
        } else if (event == RE_EVENT_PUSH) {
            printf("push\n");
        }
    }
}

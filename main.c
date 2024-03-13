#include "gpio.h"
#include "interrupts.h"
#include "uart.h"
#include "re.h"
#include "printf.h"
#include "chess.h"

#define RE_CLOCK GPIO_PB0
#define RE_DATA GPIO_PD22
#define RE_SW GPIO_PD21 // (button)

int main(void) {
    gpio_init();
    interrupts_init();
    uart_init();

    interrupts_global_enable();
    while (true) {
        read_move();
    }
}

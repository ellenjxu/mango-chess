#include "gpio.h"

void main(void) {
    gpio_init();
    gpio_set_output(GPIO_PD18);
}

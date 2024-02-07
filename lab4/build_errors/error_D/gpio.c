#include "gpio.h"

static void gpio_init() {}

void gpio_set_output(gpio_id_t id) {
    gpio_set_function(id, GPIO_FN_OUTPUT);
}
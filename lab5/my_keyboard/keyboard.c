#include "gpio.h"
#include "gpio_extra.h"
#include "keyboard.h"
#include "ps2.h"

static ps2_device_t *dev;

void keyboard_init(gpio_id_t clock_gpio, gpio_id_t data_gpio) {
    dev = ps2_new(clock_gpio, data_gpio);    // create new PS2 device
}

unsigned char keyboard_read_scancode(void) {
    return ps2_read(dev);       // read from PS2 device
}


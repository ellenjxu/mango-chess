#ifndef RE_H
#define RE_H

// gpio pins for connecting keyboard clock/data lines
#define RE_CLOCK GPIO_PG12
#define RE_DATA GPIO_PB7
#define RE_SW GPIO_PG13

#include "gpio.h"

typedef struct re_device {
    gpio_id_t clock;
    gpio_id_t data;
    gpio_id_t sw;
    // int i;  // bit index in the scancode
    // unsigned char scancode;
} re_device_t;

void re_test();

#endif

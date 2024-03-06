#ifndef RE_H
#define RE_H

/*
 * Module to drive rotary encoder. Uses three GPIO pins (clock, data and the
 * push button).
 *
 * Author: Ellen Xu <ellenjxu@stanford.edu>
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 */

#include "gpio.h"

#define RE_CLOCK GPIO_PG12
#define RE_DATA GPIO_PB7
#define RE_SW GPIO_PG13 // (button)

typedef struct re_device {
    gpio_id_t clock;
    gpio_id_t data;
    gpio_id_t sw;
    // int i;  // bit index in the scancode
    // unsigned char scancode;
} re_device_t;

void re_test();

#endif

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
#include "ringbuffer.h"

typedef struct re_device {
    gpio_id_t clock;
    gpio_id_t data;
    gpio_id_t sw;
    rb_t *rb;
    int angle;
} re_device_t;

typedef enum {
    RE_EVENT_NONE = 0,
    RE_EVENT_CLOCKWISE,
    RE_EVENT_COUNTERCLOCKWISE,
    RE_EVENT_PUSH,
} re_event_t;

/*
 * `re_new`: Create a new rotary encoder device
 *
 * Create a new rotary encoder device with the given GPIO pins.
 *
 * NOTE: Remember to call interrupts_init and interrupts_global_enable!
 */
re_device_t* re_new(gpio_id_t clock_gpio, gpio_id_t data_gpio, gpio_id_t sw_gpio);

/*
 * `re_read`: Read the next event from the rotary encoder
 *
 * Read the next event from the rotary encoder. If there is no event, it will
 * return RE_EVENT_NONE.
 */
re_event_t re_read(re_device_t* dev);

/*
 * `re_read_blocking`: Read the next event from the rotary encoder, blocking
 *
 * Read the next event from the rotary encoder. If there is no event, it will
 * block until there is one.
 */
re_event_t re_read_blocking(re_device_t* dev);

#endif

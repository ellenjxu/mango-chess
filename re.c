/*
 * Rotary encoder module for Mango Pi.
 *
 * Author: Ellen Xu <ellenjxu@stanford.edu>
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 */
#include "re.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "gpio_interrupt.h"
#include "interrupts.h"
#include "malloc.h"
#include "ringbuffer.h"
#include "timer.h"
#include <stdint.h>

void handle_clock(uintptr_t pc, void *data) {
    re_device_t* dev = (re_device_t*)data;
    int clk_state = gpio_read(dev->clock);
    int data_state = gpio_read(dev->data);
    
    if (clk_state == data_state) {
        rb_enqueue(dev->rb, RE_EVENT_CLOCKWISE);
        dev->angle--;
    } else {
        rb_enqueue(dev->rb, RE_EVENT_COUNTERCLOCKWISE);
        dev->angle++;
    }

    gpio_interrupt_clear(dev->clock);
}

void handle_button(uintptr_t pc, void *data) {
    re_device_t* dev = (re_device_t*)data;
    rb_enqueue(dev->rb, RE_EVENT_PUSH);

    gpio_interrupt_clear(dev->sw);
}

re_device_t* re_new(gpio_id_t clock_gpio, gpio_id_t data_gpio, gpio_id_t sw_gpio) {
    re_device_t* dev = malloc(sizeof(*dev));

    dev->clock = clock_gpio;
    gpio_set_input(dev->clock);
    gpio_set_pullup(dev->clock);

    dev->data = data_gpio;
    gpio_set_input(dev->data);
    gpio_set_pullup(dev->data);

    dev->sw = sw_gpio;
    gpio_set_input(dev->sw);
    gpio_set_pullup(dev->sw);

    dev->rb = rb_new();

    gpio_interrupt_init();

    gpio_interrupt_config(dev->clock, GPIO_INTERRUPT_NEGATIVE_EDGE, true);
    gpio_interrupt_register_handler(dev->clock, handle_clock, dev);
    gpio_interrupt_enable(dev->clock);

    gpio_interrupt_config(dev->sw, GPIO_INTERRUPT_NEGATIVE_EDGE, true);
    gpio_interrupt_register_handler(dev->sw, handle_button, dev);
    gpio_interrupt_enable(dev->sw);

    return dev;
}

re_event_t re_read(re_device_t* dev) {
    int result;
    if (rb_dequeue(dev->rb, &result)) {
        return result;
    } else {
        return RE_EVENT_NONE;
    }
}

re_event_t re_read_blocking(re_device_t* dev) {
    int result;
    while (!rb_dequeue(dev->rb, &result)) {}
    return result;
}


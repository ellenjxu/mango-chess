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
#include "ringbuffer_ptr.h"
#include "timer.h"
#include <stdint.h>

void handle_clock(uintptr_t pc, void *data) {
    re_device_t *dev = (re_device_t*)data;
    gpio_interrupt_clear(dev->clock);

    // read pin states
    int clk_state = gpio_read(dev->clock);
    int data_state = gpio_read(dev->data);

    // create event struct
    unsigned long now = timer_get_ticks();
    re_event_t *event = malloc(sizeof(re_event_t));
    event->ticks = now;
    
    if (clk_state == data_state) {
        // data leads, clockwise
        event->type = RE_EVENT_CLOCKWISE;
        dev->angle--;
    } else {
        // clock leads, counterclockwise
        event->type = RE_EVENT_COUNTERCLOCKWISE;
        dev->angle++;
    }

    rb_ptr_enqueue(dev->rb, (uintptr_t)event);
}

void handle_button(uintptr_t pc, void *data) {
    re_device_t* dev = (re_device_t*)data;
    gpio_interrupt_clear(dev->sw);

    // create event struct
    unsigned long now = timer_get_ticks();
    re_event_t *event = malloc(sizeof(re_event_t));

    event->ticks = now;
    event->type = RE_EVENT_PUSH;

    rb_ptr_enqueue(dev->rb, (uintptr_t)event);
}

re_device_t *re_new(gpio_id_t clock_gpio, gpio_id_t data_gpio, gpio_id_t sw_gpio) {
    re_device_t* dev = malloc(sizeof(*dev));

    // set up GPIO pins
    dev->clock = clock_gpio;
    gpio_set_input(dev->clock);
    gpio_set_pullup(dev->clock);

    dev->data = data_gpio;
    gpio_set_input(dev->data);
    gpio_set_pullup(dev->data);

    dev->sw = sw_gpio;
    gpio_set_input(dev->sw);
    gpio_set_pullup(dev->sw);

    // allocate ringbuffer for rotary encoder events (stored as pointers).
    dev->rb = rb_ptr_new();

    // set up interrupts
    // use the data pointer of the interrupt to store the deviece
    gpio_interrupt_init();

    gpio_interrupt_config(dev->clock, GPIO_INTERRUPT_NEGATIVE_EDGE, true);
    gpio_interrupt_register_handler(dev->clock, handle_clock, dev);
    gpio_interrupt_enable(dev->clock);

    gpio_interrupt_config(dev->sw, GPIO_INTERRUPT_NEGATIVE_EDGE, true);
    gpio_interrupt_register_handler(dev->sw, handle_button, dev);
    gpio_interrupt_enable(dev->sw);

    return dev;
}

re_event_t *re_read(re_device_t* dev) {
    uintptr_t result;
    if (rb_ptr_dequeue(dev->rb, &result)) {
        // success
        return (re_event_t *)result;
    } else {
        // failure
        return NULL;
    }
}

re_event_t *re_read_blocking(re_device_t* dev) {
    uintptr_t result;

    while (!rb_ptr_dequeue(dev->rb, &result)) {} // spin

    return (re_event_t *)result;
}


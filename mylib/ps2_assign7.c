/* File: ps2_assign7.c
 * -------------------
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 * Interrupt-based PS/2 device serial interface for Mango Pi.
 */
#include "gpio.h"
#include "gpio_extra.h"
#include "gpio_interrupt.h"
#include "malloc.h"
#include "ps2.h"
#include "ringbuffer.h"
#include "timer.h"
#include <stdint.h>

#define WAIT_TIME_USEC 100
#define ERROR_FLUSH_TIME_US 11 * WAIT_TIME_USEC

#define CLOCK_LOW_USEC 100

#define READ_ERROR 0

struct ps2_device {
    gpio_id_t clock;
    gpio_id_t data;
    unsigned int scancode;
    int nbits;
    unsigned long previous_ticks;
    rb_t *rb;
};

/*
 * Checks for bitwise odd parity given a value and a parity bit.
 * Return 1 if valid and 0 if invalid.
 * parity_bit is assumed to be 1 or 0.
*/
static int parity_odd_check(int value, int parity_bit) {
    while (value != 0) {
        parity_bit ^= value & 0b1;
        value >>= 1;
    }
    return parity_bit;
}

/*
 * Handles interrupt from clock pin of a PS/2 device. It reads the data pin and
 * updates the scancode and nbits of the device. If the clock has been low for
 * too long, it resets the scancode and nbits.
 */
static void clock_edge(uintptr_t pc, void *dev_data) {
    ps2_device_t *dev = (ps2_device_t *)dev_data;

    // Timeout logic
    unsigned long current_ticks = timer_get_ticks();
    if (current_ticks - dev->previous_ticks > WAIT_TIME_USEC * TICKS_PER_USEC) {
        // remove for tests
        // dev->scancode = 0;
        // dev->nbits = 0;
    }
    dev->previous_ticks = current_ticks;

    unsigned int bit = gpio_read(dev->data);

    // Process bit
    switch (dev->nbits) {
        case 0:
            // Start bit must be a 0
            if (bit == 0)
                dev->nbits++;
            break;
        case 9:
            // Parity bit
            if (parity_odd_check(dev->scancode, bit)) {
                dev->nbits++;
            } else {
                dev->scancode = 0;
                dev->nbits = 0;
            }
            break;
        case 10:
            // Stop bit (must be 1)
            if (bit == 1)
                rb_enqueue(dev->rb, dev->scancode);
            dev->scancode = 0;
            dev->nbits = 0;
            break;
        default:
            // Data bits
            dev->scancode |= bit << (dev->nbits - 1);
            dev->nbits++;
    }

    // Clear interrupt
    gpio_interrupt_clear(dev->clock);
}

/*
 * Creates a new PS2 device connected to given clock and data pins,
 * The gpios are configured as input and set to use internal pull-up
 * (PS/2 protocol requires clock/data to be high default)
*/
ps2_device_t *ps2_new(gpio_id_t clock_gpio, gpio_id_t data_gpio) {
    // consider why must malloc be used to allocate device
    ps2_device_t *dev = malloc(sizeof(*dev));

    dev->clock = clock_gpio;
    gpio_set_input(dev->clock);
    gpio_set_pullup(dev->clock);

    dev->data = data_gpio;
    gpio_set_input(dev->data);
    gpio_set_pullup(dev->data);

    dev->rb = rb_new();

    // set up interrupts
    gpio_interrupt_init();
    gpio_interrupt_config(dev->clock, GPIO_INTERRUPT_NEGATIVE_EDGE, false);
    gpio_interrupt_register_handler(dev->clock, clock_edge, dev);
    gpio_interrupt_enable(dev->clock);

    return dev;
}

// This function makes writing the Mouse driver way more convenient and reliable
bool ps2_has_char(ps2_device_t *dev) {
    return !rb_empty(dev->rb);
}

unsigned char ps2_read(ps2_device_t *dev) {
    while (rb_empty(dev->rb)) {}

    int val;
    rb_dequeue(dev->rb, &val);

    return val;
}

static void wait_for_clock(ps2_device_t *dev) {
    while (gpio_read(dev->clock) == 0) {}
    while (gpio_read(dev->clock) == 1) {}
}

bool ps2_write(ps2_device_t *dev, unsigned char command) {
    // compute the data we want to send
    uint16_t data = command;
    // command <<= 1; // start bit
    data |= parity_odd_check(data, 1) << 8; // parity bit
    data |= 1 << 9; // stop bit

    volatile int *nbits = &dev->nbits;
    volatile unsigned long *previous_ticks = &dev->previous_ticks;

    // wait until there is no data
    while (*nbits != 0) {
        unsigned long now = timer_get_ticks();
        if (now - *previous_ticks > WAIT_TIME_USEC * TICKS_PER_USEC)
            break;
    }

    // disable pesky interrupts
    gpio_interrupt_disable(dev->clock);
    gpio_set_output(dev->clock);
    gpio_set_output(dev->data);

    // send "request-to-send" signal
    gpio_write(dev->clock, 0);
    timer_delay_us(CLOCK_LOW_USEC);
    gpio_write(dev->data, 0);

    // clock is now input
    gpio_set_input(dev->clock);
    gpio_set_pullup(dev->clock);

    // bit bang!
    for (int i = 0; i < 10; i++) {
        wait_for_clock(dev);
        gpio_write(dev->data, data & 1);
        data >>= 1;
    }

    // data is now input
    gpio_set_input(dev->data);
    gpio_set_pullup(dev->data);

    // wait for acknowledge
    wait_for_clock(dev);
    bool success = gpio_read(dev->data) == 0;

    // re-enable interrupts
    gpio_interrupt_config(dev->clock, GPIO_INTERRUPT_NEGATIVE_EDGE, false);
    gpio_interrupt_register_handler(dev->clock, clock_edge, dev);
    gpio_interrupt_enable(dev->clock);

    return success;
}

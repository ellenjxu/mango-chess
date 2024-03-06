/* File: ps2_assign5.c
 * -------------------
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 * PS/2 device serial interface for Mango Pi.
 */
#include "gpio.h"
#include "gpio_extra.h"
#include "malloc.h"
#include "ps2.h"
#include "timer.h"

#define WAIT_TIME_US 1000
#define ERROR_FLUSH_TIME_US 11 * WAIT_TIME_US

#define READ_ERROR 0

struct ps2_device {
    gpio_id_t clock;
    gpio_id_t data;
};

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
    return dev;
}

/*
 * Wait until a falling edge on the clock of a PS/2 device. First wait until
 * the clock goes high, then wait a maximum of timeout_us microseconds.
 * Returns 0 if we timed out and 1 if the clock went down.
 * 
 * Warning: blocking
*/
static int wait_for_clock(ps2_device_t *dev, int timeout_us) {
    while (gpio_read(dev->clock) == 0) {}; // wait until clock goes high

    unsigned long busy_wait_until = timer_get_ticks() + WAIT_TIME_US * TICKS_PER_USEC;
    while (gpio_read(dev->clock) == 1) { // wait until clock goes low
        if (timeout_us && timer_get_ticks() > busy_wait_until) {
            // give up if too much time passes
            return 0;
        }
    } 
    return 1;
}

/*
 * Attempt to read a single bit from a PS/2 device. Return -1 if the read
 * times out. Otherwise return 1 or 0, depending on bit.
 * 
 * Warning: blocking
*/
static int read_bit(ps2_device_t *dev, int timeout) {
    if (!wait_for_clock(dev, timeout)) return -1;
    return gpio_read(dev->data);
}

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
 * Abstracts the attempted read from ps2_read. It attempts to read PS/2
 * scancode and returns READ_ERROR if there is a problem.
 * 
 * Since we get LSB first, we shift each bit left by the position it should
 * occupy in the byte.
*/
unsigned char ps2_attempt_read(ps2_device_t *dev) {
    unsigned int scancode = 0;
    
    for (int i = 0; i < 11; i++) {
        int bit = read_bit(dev, i > 0); // do not timeout first bit
        if (bit == -1) return READ_ERROR;

        if (i == 0) {           // start bit (always high)
            if (bit != 0)
                return READ_ERROR;
        } else if (i == 9) {    // parity bit
            if (!parity_odd_check(scancode, bit))
                return READ_ERROR;
        } else if (i == 10) {   // stop bit (always low)
            if (bit != 1)
                return READ_ERROR;
        } else {
            scancode |= bit << (i - 1);
        }
    }
    return scancode;
}

unsigned char ps2_read(ps2_device_t *dev) {
    while (1) {
        unsigned char scancode = ps2_attempt_read(dev);

        if (scancode != READ_ERROR)
            return scancode;
    }
}

/* File: gpio.c
 * ------------
 * Javier Garcia Nieto - jgnieto@stanford.edu
 * GPIO library for Mango Pi board.
 */
#include "gpio.h"
#include <stddef.h>

enum { GROUP_B = 0, GROUP_C, GROUP_D, GROUP_E, GROUP_F, GROUP_G };

typedef struct  {
    unsigned int group;
    unsigned int pin_index;
} gpio_pin_t;

// The gpio_id_t enumeration assigns a symbolic constant for each
// in such a way to use a single hex constant. The more significant
// hex digit identifies the group and lower 2 hex digits are pin index:
//       constant 0xNnn  N = which group,  nn = pin index within group
//
// This helper function extracts the group and pin index from a gpio_id_t
// e.g. GPIO_PB4 belongs to GROUP_B and has pin_index 4
static gpio_pin_t get_group_and_index(gpio_id_t gpio) {
    gpio_pin_t gp;
    gp.group = gpio >> 8;
    gp.pin_index = gpio & 0xff; // lower 2 hex digits
    return gp;
}

// The gpio groups are differently sized, e.g. B has 13 pins, C only 8.
// This helper function confirms that a gpio_id_t is valid (group
// and pin index are valid)
bool gpio_id_is_valid(gpio_id_t pin) {
    gpio_pin_t gp = get_group_and_index(pin);
    switch (gp.group) {
        case GROUP_B: return (gp.pin_index <= GPIO_PB_LAST_INDEX);
        case GROUP_C: return (gp.pin_index <= GPIO_PC_LAST_INDEX);
        case GROUP_D: return (gp.pin_index <= GPIO_PD_LAST_INDEX);
        case GROUP_E: return (gp.pin_index <= GPIO_PE_LAST_INDEX);
        case GROUP_F: return (gp.pin_index <= GPIO_PF_LAST_INDEX);
        case GROUP_G: return (gp.pin_index <= GPIO_PG_LAST_INDEX);
        default:      return false;
    }
}

// This helper function is suggested to return the address of
// the config0 register for a gpio group, i.e. get_cfg0_reg(GROUP_B)
// Refer to the D1 user manual to learn the address the config0 register
// for each group. Be sure to note how the address of the config1 and
// config2 register can be computed as relative offset from config0.
static volatile unsigned int *get_cfg0_reg(unsigned int group) {
    // base address of config0 register for PB_0
    unsigned int *addr = (unsigned int *)0x02000030; 

    // add offset for higher groups
    // (divide by 4 because C multiplies times 4 for unsigned int)
    return addr + group * 0x30 / 4;
}

// This helper function is suggested to return the address of
// the data register for a gpio group. Refer to the D1 user manual
// to learn the address of the data register for each group.
static volatile unsigned int *get_data_reg(unsigned int group) {
    // compute based on cfg0 register address
    // (divide by 4 because C multiplies times 4 for unsigned int)
    return get_cfg0_reg(group) + 0x10 / 4;
}

void gpio_init(void) {
    // no initialization required for this peripheral
}

void gpio_set_input(gpio_id_t pin) {
    gpio_set_function(pin, GPIO_FN_INPUT);
}

void gpio_set_output(gpio_id_t pin) {
    gpio_set_function(pin, GPIO_FN_OUTPUT);
}

/*
 * Access memory-mapped GPIO registers to configure a pin.
 * `pin` is a GPIO_P*
 * `function` is one of the GPIO_FN_* constants
*/
void gpio_set_function(gpio_id_t pin, unsigned int function) {
    if (!gpio_id_is_valid(pin)) return;

    // check if function uses bits that are not the rightmost 4.
    if (function >> 4 != 0) return;

    // compute group and pin index from gpio_id_t
    gpio_pin_t gp = get_group_and_index(pin);

    // compute cfg register address (add offset for higher groups)
    volatile unsigned int *cfg_reg = get_cfg0_reg(gp.group) + gp.pin_index / 8;

    // unseful value to shift the function bits
    unsigned char bit_index = (gp.pin_index % 8) * 4;

    // set the function bits to the desired value without overwriting other pins
    *cfg_reg = (*cfg_reg & ~(0xf << bit_index)) | (function << bit_index);
}

/*
 * Read from memory-mapped GPIO registers to get the current function of a pin.
 * `pin` is a GPIO_P*
*/
unsigned int gpio_get_function(gpio_id_t pin) {
    if (!gpio_id_is_valid(pin)) return GPIO_INVALID_REQUEST;

    // compute group and pin index from gpio_id_t
    gpio_pin_t gp = get_group_and_index(pin);

    // compute cfg register address (add offset for higher groups)
    volatile unsigned int *cfg_reg = get_cfg0_reg(gp.group) + gp.pin_index / 8;

    // read value from memory and mask the bits corresponding to the pin
    return (*cfg_reg >> (gp.pin_index % 8) * 4) & 0b1111;
}

/*
 * Set the output value of a pin (1 = high, 0 = low).
 * `pin` is a GPIO_P*
 * `value` is 1 or 0 (any non-zero value is considered high)
*/
void gpio_write(gpio_id_t pin, int value) {
    if (!gpio_id_is_valid(pin)) return;

    // compute group and pin index from gpio_id_t
    gpio_pin_t gp = get_group_and_index(pin);

    // compute data register address
    volatile unsigned int *data_reg = get_data_reg(gp.group);

    if (value != 0) value = 1; // any non-zero value is considered high

    // set the bit corresponding to the pin to the desired value without overwriting other pins
    *data_reg = (*data_reg & ~(0b1 << gp.pin_index)) | (value << gp.pin_index);
}

/*
 * Read the input value of a pin (1 = high, 0 = low).
 * `pin` is a GPIO_P*
*/
int gpio_read(gpio_id_t pin) {
    if (!gpio_id_is_valid(pin)) return GPIO_INVALID_REQUEST;

    // compute group and pin index from gpio_id_t
    gpio_pin_t gp = get_group_and_index(pin);

    // compute data register address
    volatile unsigned int *data_reg = get_data_reg(gp.group);
    
    // read value and mask appropriate bit
    return (*data_reg >> gp.pin_index) & 0b1;
}

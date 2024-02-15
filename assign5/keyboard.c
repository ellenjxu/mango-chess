/* File: keyboard.c
 * -----------------
 * ***** TODO: add your file header comment here *****
 */
#include "keyboard.h"
#include "ps2.h"

static ps2_device_t *dev;

void keyboard_init(gpio_id_t clock_gpio, gpio_id_t data_gpio) {
    dev = ps2_new(clock_gpio, data_gpio);
}

unsigned char keyboard_read_scancode(void) {
    return ps2_read(dev);
}

key_action_t keyboard_read_sequence(void) {
    /***** TODO: Your code goes here *****/
    key_action_t action = { 0 };
    return action;
}

key_event_t keyboard_read_event(void) {
    /***** TODO: Your code goes here *****/
    key_event_t event = { 0 };
    return event;
}

unsigned char keyboard_read_next(void) {
    /***** TODO: Your code goes here *****/
    return '!';
}

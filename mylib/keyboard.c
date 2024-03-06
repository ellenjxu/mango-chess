/* File: keyboard.c
 * -----------------
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 * PS/2 keyboard interface for Mango Pi.
 */
#include "keyboard.h"
#include "ps2.h"
#include "ps2_keys.h"

static ps2_device_t *dev;
static keyboard_modifiers_t modifiers;

#define FIRST_MOD_KEY PS2_KEY_SHIFT
#define LAST_MOD_KEY PS2_KEY_CAPS_LOCK

void keyboard_init(gpio_id_t clock_gpio, gpio_id_t data_gpio) {
    dev = ps2_new(clock_gpio, data_gpio);
}

unsigned char keyboard_read_scancode(void) {
    return ps2_read(dev);
}

key_action_t keyboard_read_sequence(void) {
    key_action_t action = { KEY_PRESS, 0 }; // default to KEY_PRESS

    while (1) {
        unsigned char scancode = ps2_read(dev);

        if (scancode == PS2_CODE_EXTENDED)
            continue;
        
        if (scancode == PS2_CODE_RELEASE) {
            action.what = KEY_RELEASE;
            continue;
        }

        action.keycode = scancode;
        break;
    }

    return action;
}

key_event_t keyboard_read_event(void) {
    key_event_t event;
    key_action_t action;
    while (1) { // loop until we find a non-modifier key
        action = keyboard_read_sequence();
        ps2_key_t key = ps2_keys[action.keycode];

        // capture shift, alt, ctrl, and caps lock
        if (FIRST_MOD_KEY <= key.ch && key.ch <= LAST_MOD_KEY) {
            if (key.ch == PS2_KEY_CAPS_LOCK) { // caps lock is sticky
                if (action.what == KEY_PRESS)
                    modifiers ^= KEYBOARD_MOD_CAPS_LOCK;
            } else { // other mod keys have to be held down
                if (action.what == KEY_PRESS)
                    modifiers |= 1 << (key.ch - FIRST_MOD_KEY);
                else
                    modifiers &= ~(1 << (key.ch - FIRST_MOD_KEY));
            }
        } else { // regular character
            event.action = action;
            event.key = key;
            event.modifiers = modifiers;
            break;
        }
    }
    return event;
}

unsigned char keyboard_read_next(void) {
    unsigned char ch;
    while (1) {
        key_event_t event = keyboard_read_event();

        if (event.action.what != KEY_PRESS) // don't care about key release
            continue;

        ch = event.key.ch;

        if (event.modifiers & KEYBOARD_MOD_SHIFT) {
            // if shift is pressed, use other_ch
            ch = event.key.other_ch;
        } else if (event.modifiers & KEYBOARD_MOD_CAPS_LOCK && 'a' <= ch && ch <= 'z') {
            // caps lock is ignored if modifier key
            ch = event.key.other_ch;
        }

        break;
    }

    return ch;
}

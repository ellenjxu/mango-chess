#include "mouse.h"
#include "ps2.h"
#include "timer.h"
#include "uart.h"
#include "printf.h"

#define TIMEOUT_USEC 2000

static struct {
    ps2_device_t *device;
    unsigned char previous_flags;
} module;

extern bool ps2_has_char(ps2_device_t *dev);

void mouse_init(gpio_id_t clock, gpio_id_t data) {
    uart_putstring("Initializing mouse...\n");
    module.device = ps2_new(clock, data);
    while (1) {
        // Clear any data that may be in the buffer
        while (ps2_has_char(module.device)) {
            ps2_read(module.device);
        }

        // Reset the mouse
        while (!ps2_write(module.device, 0xFF)) {}

        // Wait to get the acknowledgement
        timer_delay_ms(1500);

        // Set the mouse to stream mode
        while (!ps2_write(module.device, 0xF4)) {}

        // Wait to get the acknowledgement
        timer_delay_ms(10);

        // Ensure we read an acknowledgement
        int i = 0;
        while (ps2_has_char(module.device) && ++i < 5) {
            if (ps2_read(module.device) == 0xFA) {
                return;
            }
        }
    }
}

/*
 * Reads 3 bytes of data from the mouse, ensuring the are all part of the same
 * packet.
 */
static void read_data(unsigned char *buf) {
    unsigned long last_tick = 0;
    int nbytes = 0;
    while (nbytes < 3) {
        unsigned char byte = ps2_read(module.device);

        // Timeout logic
        unsigned long now = timer_get_ticks();
        if (now - last_tick > TIMEOUT_USEC * TICKS_PER_USEC) {
            nbytes = 0;
        }
        last_tick = now;

        // Store byte to buffer
        buf[nbytes++] = byte;
    }
}

mouse_event_t mouse_read_event(void) {
    // Read the 3 bytes of data from the mouse
    unsigned char buf[3];
    read_data(buf);
    unsigned char flags = buf[0];
    int dx = buf[1];
    int dy = buf[2];

    // Parse binary data
    mouse_event_t event;
    event.left = flags & 1;
    event.right = (flags >> 1) & 1;
    event.middle = (flags >> 2) & 1;
    event.x_overflow = (flags >> 6) & 1;
    event.y_overflow = (flags >> 7) & 1;

    // Parse event
    char buttons_diff = (flags & 0b111) - (module.previous_flags & 0b111);
    if (buttons_diff < 0) {
        event.action = MOUSE_BUTTON_RELEASE;
    } else if (buttons_diff > 0) {
        event.action = MOUSE_BUTTON_PRESS;
    } else {
        event.action = flags & 0b111 ? MOUSE_DRAGGED : MOUSE_MOVED;
    }
    module.previous_flags = flags;

    // Two's complement, subtract 256 if sign bit is set
    if ((flags >> 4) & 1) {
        dx -= 256;
    }

    if ((flags >> 5) & 1) {
        dy -= 256;
    }

    event.dx = dx;
    event.dy = dy;

    module.previous_flags = flags;

    return event;
}

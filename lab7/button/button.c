#include "gl.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "gpio_interrupt.h"
#include "interrupts.h"
#include "printf.h"
#include "ringbuffer.h"
#include "uart.h"

static const gpio_id_t BUTTON = GPIO_PB4;
static int gCount = 0;

void wait_for_click(void) {
    // TODO: implement this function
    // wait for falling edge on button
    // increment gCount
    // uart_putstring
}

void redraw(int nclicks) {
    static int nredraw = -1;
    // count number of redraws, alternate bg color on each redraw
    color_t bg = nredraw++ % 2 ? GL_AMBER : GL_BLUE;

    gl_clear(GL_BLACK);
    char buf[100];
    snprintf(buf, sizeof(buf), "Click count = %d (redraw #%d)", nclicks, nredraw);
    gl_draw_string(0, 0, buf, GL_WHITE);
    // intentionally slow loop for educational purposes :-)
    for (int y = gl_get_char_height(); y < gl_get_height(); y++) {
        for (int x = 0; x < gl_get_width(); x++) {
            gl_draw_pixel(x, y, bg);
        }
    }
}

void main(void) {
    gpio_init();
    uart_init();
    gl_init(800, 600, GL_SINGLEBUFFER);

    gpio_set_input(BUTTON); // configure button
    gpio_set_pullup(BUTTON);

    int drawn = 0;
    redraw(drawn);
    while (true) {
        wait_for_click();
        drawn = gCount;
        redraw(drawn);
    }
}

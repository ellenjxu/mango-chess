/* File speed.c
 * ------------
 * Simple main program with timing code to measure
 * performance of different implementations of redraw.
 */

#include "gl.h"
#include "printf.h"
#include "timer.h"
#include "uart.h"

// -------------------------------------------------------------------
// Time Trial Helpers
// -------------------------------------------------------------------


static void wait_for_user(const char *msg) {
    printf("%s: ", msg);
    char ch = uart_getchar();
    printf("%c\n", ch);
}

static unsigned long _time_trial(void (*fn)(color_t c)) {
    static int nrefresh = 0;
    color_t cycle[3] = {GL_RED, GL_WHITE, GL_BLUE};
    color_t c = cycle[nrefresh++ % 3];

    gl_clear(0xff555555);
    wait_for_user("type any key to start");
    unsigned long start = timer_get_ticks();
    fn(c);
    unsigned long elapsed = timer_get_ticks() - start;
    return elapsed;
}

#define TIME_TRIAL(fn) do {           \
    printf("Will run " #fn "... ");    \
    printf("took %ld ticks\n", _time_trial(fn)); \
} while (0)


// -------------------------------------------------------------------
// Baseline redraw0, correct implementation but pokey
// -------------------------------------------------------------------

static void redraw0(color_t c) {
    for (int y = 0; y < gl_get_height(); y++) {
        for (int x = 0; x < gl_get_width(); x++) {
            gl_draw_pixel(x, y, c);
        }
    }
}

// -------------------------------------------------------------------
// redraw1, only call getters once outside loop
// -------------------------------------------------------------------

static void redraw1(color_t c) {
    int h = gl_get_height();
    int w = gl_get_width();
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            gl_draw_pixel(x, y, c);
        }
    }
}

// -------------------------------------------------------------------
// Improved redrawN functions of your own go here:
// -------------------------------------------------------------------

void main(void)  {
    timer_init();
    uart_init();
    gl_init(1280, 720, GL_SINGLEBUFFER);

    printf("\nStarting time trials now.\n");

    TIME_TRIAL(redraw0);
    TIME_TRIAL(redraw1);
    // TODO: Add more TIME_TRIAL calls here for your improved versions

    printf("\nAll done with time trials.\n");
    wait_for_user("type any key to exit");
}

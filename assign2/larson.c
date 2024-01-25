/* File: larson.c
 * --------------
 * Sample program that demonstrates use of gpio and timer library modules.
 * (Larson scanner rewritten in C)
 * Author: Julie Zelenski <zelenski@cs.stanford.edu>
 */
#include "gpio.h"
#include "timer.h"

#define NUM_LEDS 4
#define DELAY_MS  (1000/NUM_LEDS)  // frequency 1 sweep per sec

void blink(gpio_id_t pin) {
    gpio_write(pin, 1);
    timer_delay_ms(DELAY_MS);
    gpio_write(pin, 0);
    timer_delay_ms(DELAY_MS);
}

void main(void) {
    gpio_id_t leds[NUM_LEDS] = { GPIO_PB0, GPIO_PB1, GPIO_PB2, GPIO_PB3 };
    gpio_init();
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_set_output(leds[i]);
    }

    int cur = 0;
    while (1) {
        for (int i = 0; i < NUM_LEDS-1; i++) {
            blink(leds[cur++]); // move up
        }
        for (int i = 0; i < NUM_LEDS-1; i++) {
            blink(leds[cur--]); // move down
        }
    }
}

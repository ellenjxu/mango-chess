/* File: timer.c
 * -------------
 * Javier Garcia Nieto - jgnieto@stanford.edu
 * Timer libary for Mango Pi board.
 */
#include "timer.h"

// implemented in file timer_asm.s
extern unsigned long timer_get_ticks(void);

void timer_init(void) {
    // no initialization required for this peripheral
}

void timer_delay(int secs) {
    timer_delay_us(secs*1000000);
}

void timer_delay_ms(int ms) {
    timer_delay_us(ms*1000);
}

void timer_delay_us(int us) {
    unsigned long busy_wait_until = timer_get_ticks() + us * TICKS_PER_USEC;
    while (timer_get_ticks() < busy_wait_until) {} // spin
}
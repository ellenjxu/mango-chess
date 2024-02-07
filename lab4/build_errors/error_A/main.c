#include "mango.h"
#include "timer.h"

#define DELAY 1000;

void main(void) {
    while (1) {
        mango_actled(LED_ON);
        timer_delay_ms(DELAY);
    }
}

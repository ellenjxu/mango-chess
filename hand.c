#include "assert.h"
#include "chess_commands.h"
#include "gpio.h"
#include "interrupts.h"
#include "jnxu.h"
#include "malloc.h"
#include "printf.h"
#include "ringbuffer.h"
#include "re.h"
#include "timer.h"
#include "uart.h"
#include <stdint.h>

#define RE_CLOCK GPIO_PB0
#define RE_DATA GPIO_PD22
#define RE_SW GPIO_PD21 // (button)

#define RE_TIMEOUT_USEC (200 * 1000) // 200ms

#define MIN_TICKS   4 // minimum number of RE events to treat as turn

#define SERVO_PIN GPIO_PB1

#define MGPIA_MAC "685E1C4C31FD"
#define MGPIB_MAC "685E1C4C0016"

#define BT_MODE BT_EXT_ROLE_PRIMARY
#define BT_MAC  MGPIA_MAC

#define TICKS_PER_SECOND    (1000 * 1000 * TICKS_PER_USEC)

#define SERVO_PULSE_LONG_USEC   1000
#define SERVO_PULSE_SHORT_USEC  1200
#define SERVO_PERIOD_USEC       20 * 1000

#define BUZZ_DURATION_TICKS             (TICKS_PER_SECOND / 4)
#define BUZZ_WAIT_DURATION_TICKS        (TICKS_PER_SECOND / 6)
#define LAST_BUZZ_DURATION_TICKS        (TICKS_PER_SECOND / 2)
#define LONG_BUZZ_DURATION_TICKS        (TICKS_PER_SECOND / 1)
#define LONG_BUZZ_WAIT_DURATION_TICKS   (TICKS_PER_SECOND / 1)

#define MAX(a, b) ((a) > (b) ? (a) : (b))

enum {
    BUZZ_IDLE = 0,
    BUZZ,
    LAST_BUZZ,
    LONG_BUZZ,
    BUZZ_WAIT,
    LONG_BUZZ_WAIT,
};

static struct {
    re_device_t *re;
    rb_t *buzzes;
} module;

static void enqueue_buzzes(int n) {
    if (n < 0) {
        rb_enqueue(module.buzzes, LONG_BUZZ);
        rb_enqueue(module.buzzes, BUZZ_WAIT);
        n = -n;
    }

    printf("Enqueueing %d\n", n);

    // one additional buzz
    rb_enqueue(module.buzzes, BUZZ);

    while (n--) {
        rb_enqueue(module.buzzes, BUZZ_WAIT);
        if (n == 0)
            rb_enqueue(module.buzzes, LAST_BUZZ);
        else
            rb_enqueue(module.buzzes, BUZZ);
    }

    rb_enqueue(module.buzzes, LONG_BUZZ_WAIT);
}

static void move_handler(void *aux_data, const uint8_t *message, size_t len) {
    if (len < 5 || (message[4] != '\n' && message[5] != '\n'))
        return;

    int col0 = message[0] - 'a';
    int row0 = message[1] - '1';

    int col1 = message[2] - 'a';
    int row1 = message[3] - '1';

#if PLAYING == BLACK
    enqueue_buzzes(7 - col0);
    enqueue_buzzes(7 - row0);
#else
    enqueue_buzzes(col0);
    enqueue_buzzes(row0);
#endif

    enqueue_buzzes(PLAYING * (col1 - col0));
    enqueue_buzzes(PLAYING * (row1 - row0));
}

int main(void) {
    gpio_init();
    uart_init();
    interrupts_init();
    interrupts_global_enable();

    module.buzzes = rb_new();
    module.re = re_new(RE_CLOCK, RE_DATA, RE_SW);

    gpio_set_output(SERVO_PIN);

    jnxu_init(BT_MODE, BT_MAC);
    jnxu_register_handler(CMD_MOVE, move_handler, NULL);

    // In ticks
    unsigned long buzzer_start = 0;
    unsigned long buzzer_duration = 0;

    unsigned long buzzer_next_pulse_time = 0;

    int buzzer_pulse_a = SERVO_PULSE_LONG_USEC;
    int buzzer_pulse_b = SERVO_PULSE_SHORT_USEC;

    int buzzer_status = BUZZ_IDLE;
    bool buzz_waiting = false;

    int cw = 0;
    int ccw = 0;
    unsigned long last_re_event = 0;

    while (1) {
        // rotary encoder
        re_event_t *event = re_read(module.re);

        while (event) {
last_update:
            if (event->ticks - last_re_event > RE_TIMEOUT_USEC * TICKS_PER_USEC) {
                if (MAX(ccw, cw) > MIN_TICKS) {
                    // send cursor packet
                    uint8_t motion;
                    if (cw > ccw)
                        motion = MOTION_CW;
                    else
                        motion = MOTION_CCW;

                    uint8_t buf[] = { motion };

                    jnxu_send(CMD_CURSOR, buf, sizeof(buf));
                }

                cw = ccw = 0;
            }

            switch (event->type) {
                case RE_EVENT_CLOCKWISE:
                    printf("+\n");
                    cw++;
                    last_re_event = event->ticks;
                    break;

                case RE_EVENT_COUNTERCLOCKWISE:
                    printf("-\n");
                    ccw++;
                    last_re_event = event->ticks;
                    break;

                case RE_EVENT_PUSH:
                    {
                        printf("Sending Button\n");
                        jnxu_send(CMD_PRESS, NULL, 0);
                    }
                    break;

                case RE_EVENT_NONE:
                    goto back_from_last_update;

                default:
                    break;
            }

            free(event);
            event = re_read(module.re);
        }

        re_event_t phony_event = {
            .ticks = timer_get_ticks(),
            .type = RE_EVENT_NONE
        };

        event = &phony_event;
        goto last_update;
back_from_last_update:

        // buzzer
        if (buzzer_status != BUZZ_IDLE) {
            unsigned long now = timer_get_ticks();
            unsigned long dt = now - buzzer_start;

            if (dt > buzzer_duration) {
                buzzer_status = BUZZ_IDLE;
            } else if (!buzz_waiting) {
                if (now > buzzer_next_pulse_time) {
                    gpio_write(SERVO_PIN, 1);
                    timer_delay_us(buzzer_pulse_a);
                    gpio_write(SERVO_PIN, 0);

                    buzzer_next_pulse_time = timer_get_ticks() + (
                            SERVO_PERIOD_USEC - buzzer_pulse_a
                            ) * TICKS_PER_USEC;

                    int tmp = buzzer_pulse_a;
                    buzzer_pulse_a = buzzer_pulse_b;
                    buzzer_pulse_b = tmp;
                }
            }
        } else if (!rb_empty(module.buzzes)) {
            assert(rb_dequeue(module.buzzes, &buzzer_status));

            buzzer_start = timer_get_ticks();

            switch (buzzer_status) {
                case BUZZ:
                    buzzer_duration = BUZZ_DURATION_TICKS;
                    buzz_waiting = false;
                    break;
                
                case LAST_BUZZ:
                    buzzer_duration = LAST_BUZZ_DURATION_TICKS;
                    buzz_waiting = false;
                    break;

                case LONG_BUZZ:
                    buzzer_duration = LONG_BUZZ_DURATION_TICKS;
                    buzz_waiting = false;
                    break;

                case BUZZ_WAIT:
                    buzzer_duration = BUZZ_WAIT_DURATION_TICKS;
                    buzz_waiting = true;
                    break;

                case LONG_BUZZ_WAIT:
                    buzzer_duration = LONG_BUZZ_WAIT_DURATION_TICKS;
                    buzz_waiting = true;
                    break;
            }
        }
    }
}

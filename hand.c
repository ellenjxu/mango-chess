#include "assert.h"
#include "chess_commands.h"
#include "gpio.h"
#include "interrupts.h"
#include "jnxu.h"
#include "ringbuffer.h"
#include "re.h"
#include "timer.h"
#include "uart.h"
#include <stdint.h>

#define RE_CLOCK GPIO_PB0
#define RE_DATA GPIO_PD22
#define RE_SW GPIO_PD21 // (button)

#define RE_TIMEOUT_USEC (200 * 1000) // 200ms

#define MIN_TICKS   10 // minimum number of RE events to treat as turn

#define SERVO_PIN GPIO_PB0

#define MGPIA_MAC "685E1C4C31FD"
#define MGPIB_MAC "685E1C4C0016"

#define BT_MODE BT_EXT_ROLE_PRIMARY
#define BT_MAC  MGPIA_MAC

#define BLACK -1
#define WHITE 1

#define PLAYING BLACK

#define TICKS_PER_SECOND    (1000 * 1000 * TICKS_PER_USEC)
#define BUZZER_PERIOD(f)    (TICKS_PER_SECOND / f)

#define BUZZ_FREQ_HZ        10
#define LONG_BUZZ_FREQ_HZ   10

#define BUZZ_DURATION_USEC      (TICKS_PER_SECOND / 4)
#define LONG_BUZZ_DURATION_USEC (TICKS_PER_SECOND / 2)
#define WAIT_BUZZ_DURATION_USEC (TICKS_PER_SECOND / 4)

#define CLAMP(x, min, max) ((x) > (max) ? (max) : ((x) < (min) ? (min) : (x)))

enum {
    BUZZ_IDLE,
    BUZZ,
    LONG_BUZZ,
    BUZZ_WAIT,
};

static struct {
    re_device_t *re;
    rb_t *buzzes;
} module;

static void enqueue_buzzes(int n) {
    if (n < 0) {
        rb_enqueue(module.buzzes, LONG_BUZZ);
        n = -n;
    }

    // one additional buzz
    rb_enqueue(module.buzzes, BUZZ);

    while (n--) {
        rb_enqueue(module.buzzes, BUZZ);
    }

    rb_enqueue(module.buzzes, BUZZ_WAIT);
}

static void move_handler(void *aux_data, const uint8_t *message, size_t len) {
    if (len < 5 || message[len - 1] != '\n')
        return;

    int col0 = message[0] - 'a';
    int row0 = message[1] - '1';

    int col1 = message[2] - 'a';
    int row1 = message[3] - '1';

    enqueue_buzzes(col0);

#if PLAYING == BLACK
    enqueue_buzzes(7 - row0);
#else
    enqueue_buzzes(row0);
#endif

    enqueue_buzzes(col1 - col0);
    enqueue_buzzes(PLAYING * (row1 - row0));
}

int main(void) {
    gpio_init();
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
    unsigned long buzzer_period = 0;
    unsigned long last_buzzer_mod = 0;
    int buzzer_status = BUZZ_IDLE;

    int cw = 0;
    int ccw = 0;
    unsigned long last_re_event = 0;

    int cursor = 0;

    while (1) {
        // rotary encoder
        re_event_t event = re_read(module.re);

        switch (event) {
            case RE_EVENT_CLOCKWISE:
                cw++;
                last_re_event = timer_get_ticks();
                break;

            case RE_EVENT_COUNTERCLOCKWISE:
                ccw++;
                last_re_event = timer_get_ticks();
                break;

            case RE_EVENT_PUSH:
                {
                    uint8_t buf[] = { cursor & 0xFF };
                    jnxu_send(CMD_CURSOR, buf, sizeof(buf));
                    cursor = 0;
                }
                break;

            case RE_EVENT_NONE:
            default:
                if (timer_get_ticks() - last_re_event > RE_TIMEOUT_USEC * TICKS_PER_USEC) {
                    if (ccw > cw)
                        cursor--;
                    else
                        cursor++;

                    cursor = CLAMP(cursor, 0, 7);

                    // send cursor packet
                    uint8_t buf[] = { cursor & 0xFF };
                    jnxu_send(CMD_CURSOR, buf, sizeof(buf));
                }
                break;
        }

        // buzzer
        if (buzzer_status != BUZZ_IDLE) {
            unsigned long dt = timer_get_ticks() - buzzer_start;
            if (dt > buzzer_duration) {
                buzzer_status = BUZZ_IDLE;
            } else {
                unsigned long mod = dt % buzzer_period;
                if (mod < last_buzzer_mod) {
                    gpio_write(SERVO_PIN, !gpio_read(SERVO_PIN));
                }
                last_buzzer_mod = mod;
            }
        } else if (!rb_empty(module.buzzes)) {
            assert(rb_dequeue(module.buzzes, &buzzer_status));

            buzzer_start = timer_get_ticks();

            switch (buzzer_status) {
                case BUZZ:
                    buzzer_duration = BUZZ_DURATION_USEC;
                    buzzer_period = BUZZER_PERIOD(BUZZ_FREQ_HZ);
                    last_buzzer_mod = buzzer_period;
                    break;
                
                case LONG_BUZZ:
                    buzzer_duration = LONG_BUZZ_DURATION_USEC;
                    buzzer_period = BUZZER_PERIOD(LONG_BUZZ_FREQ_HZ);
                    last_buzzer_mod = buzzer_period;
                    break;

                case BUZZ_WAIT:
                    buzzer_duration = WAIT_BUZZ_DURATION_USEC;
                    break;
            }
        }
    }
}


#include "assert.h"
#include "chess_commands.h"
#include "gpio.h"
#include "interrupts.h"
#include "jnxu.h"
#include "ringbuffer.h"
#include "re.h"
#include "timer.h"
#include "uart.h"

#define RE_CLOCK GPIO_PB0
#define RE_DATA GPIO_PD22
#define RE_SW GPIO_PD21 // (button)

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
    unsigned long buzzer_start = 0, buzzer_duration = 0, buzzer_period = 0;
    int playing_buzzer = BUZZ_IDLE;

    while (1) {
        // rotary encoder
        re_event_t event = re_read(module.re);

        switch (event) {
            case RE_EVENT_CLOCKWISE:
                break;

            case RE_EVENT_COUNTERCLOCKWISE:
                break;

            case RE_EVENT_PUSH:
                break;

            case RE_EVENT_NONE:
            default:
                break;
        }

        // buzzer
        if (playing_buzzer != BUZZ_IDLE) {
            // TODO
            
        } else if (!rb_empty(module.buzzes)) {
            assert(rb_dequeue(module.buzzes, &playing_buzzer));

            buzzer_start = timer_get_ticks();

            switch (playing_buzzer) {
                case BUZZ:
                    buzzer_duration = BUZZ_DURATION_USEC;
                    buzzer_period = BUZZER_PERIOD(BUZZ_FREQ_HZ);
                    break;
                
                case LONG_BUZZ:
                    buzzer_duration = LONG_BUZZ_DURATION_USEC;
                    buzzer_period = BUZZER_PERIOD(LONG_BUZZ_FREQ_HZ);
                    break;

                case BUZZ_WAIT:
                    buzzer_duration = WAIT_BUZZ_DURATION_USEC;
                    buzzer_period = WAIT_BUZZ_DURATION_USEC - 1;
                    break;
            }
        }
    }
}


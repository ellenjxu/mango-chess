#include "re.h"
#include "uart.h"
#include "printf.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "interrupts.h"
#include "malloc.h"

static re_device_t *dev;

re_device_t* re_new(gpio_id_t clock_gpio, gpio_id_t data_gpio, gpio_id_t sw_gpio) {
    re_device_t* dev = malloc(sizeof(*dev));

    dev->clock = clock_gpio;
    gpio_set_input(dev->clock);
    gpio_set_pullup(dev->clock);

    dev->data = data_gpio;
    gpio_set_input(dev->data);
    gpio_set_pullup(dev->data);

    dev->sw = sw_gpio;
    gpio_set_input(dev->sw);
    gpio_set_pullup(dev->sw);

    // gpio_interrupt_init();
    // gpio_interrupt_config(dev->clock, GPIO_INTERRUPT_NEGATIVE_EDGE, false);
    // gpio_interrupt_register_handler(dev->clock, handle_read_bit, dev);
    // gpio_interrupt_enable(dev->clock);
    // interrupts_global_enable();

    return dev;
}

void re_test(void) {
    printf("hi\n");

    dev = re_new(RE_CLOCK, RE_DATA, RE_SW);
    while (1) {
        int b = gpio_read(dev->clock); // goes low when midturn
    }
}

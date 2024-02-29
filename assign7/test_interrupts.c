/* File: test_interrupts.c
 * -----------------------
 * ***** TODO: add your file header comment here *****
 */
#include "gpio.h"
#include "gpio_extra.h"
#include "gpio_interrupt.h"
#include "keyboard.h"
#include "printf.h"
#include "timer.h"
#include "uart.h"

/*
 * This function tests the behavior of the assign5 ps2
 * implementation versus the new-improved assign7 version. If using the
 * ps2 module as written for assign5, a scancode that arrives while the main program
 * is waiting in delay is simply dropped. Once you upgrade your
 * ps2 module for assign7 to be interrupt-driven, those scancodes should
 * be queued up and can be read after delay finishes.
 */
static void check_read_delay(void) {
    keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);
    printf("\n%s() will read each typed key and pause for a few seconds\n", __func__);
    while (1) {
        printf("\nType a key on PS/2 keyboard (q to quit): ");
        char ch = keyboard_read_next();
        printf("\nRead: %c\n", ch);
        if (ch == 'q') break;
        printf("Pausing for 2 seconds (type ahead now on PS/2 keyboard to queue events)... ");
        timer_delay(2);
        printf("done.\n");
    }
}

static void clock_edge(uintptr_t pc, void *clientData) {
    static int count = 0;
    gpio_interrupt_clear(KEYBOARD_CLOCK);
    uart_putchar('0' + count); // print digit
    if (++count == 11) {// after 11 edges = one scancode, reset counter
        count = 0;
        uart_putchar('\n');
    }
}

/*
 * This function is a simple test to confirm interrupts are being generated
 * and that the interrupt handler is being called. It configures interrupts
 * on the clock gpio using the handler function above a counter.
 */
static void check_interrupts_received(void) {
    gpio_interrupt_init();
    gpio_interrupt_config(KEYBOARD_CLOCK, GPIO_INTERRUPT_NEGATIVE_EDGE, false);
    gpio_interrupt_register_handler(KEYBOARD_CLOCK, clock_edge, NULL);
    gpio_interrupt_enable(KEYBOARD_CLOCK);
    interrupts_global_enable();

    printf("\n%s() waiting for interrupts on keyboard clock gpio\n", __func__);
    printf("Type on your PS/2 keyboard. I'll wait for 5 seconds...\n");
    timer_delay(5);
    printf("Time's up!\n");
}

void main(void) {
    gpio_init();
    timer_init();
    uart_init();
    printf("\nStarting main() in %s\n", __FILE__);
    interrupts_init();

    check_interrupts_received();
    check_read_delay();
    printf("\nCompleted execution of main() in %s\n", __FILE__);
}

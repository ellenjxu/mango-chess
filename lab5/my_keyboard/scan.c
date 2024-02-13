#include "gpio.h"
#include "keyboard.h"
#include "printf.h"
#include "uart.h"

#define ESC_SCANCODE 0x76

static void test_keyboard_scancodes(void) {
    printf("\nNow reading single scancodes. Type ESC to finish this test.\n");
    while (1) {
        unsigned char scancode = keyboard_read_scancode();
        printf("[%02x]\n", scancode);
        if (scancode == ESC_SCANCODE) break;
    }
    printf("\nDone with scancode test.\n");
}

void main(void) {
    gpio_init();
    keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);
    uart_init();

    test_keyboard_scancodes();
    printf("All done!\n");
}


/* File: console_shell.c
 * ---------------------
 * This program runs the shell application. Input is read from the
 * the PS/2 keyboard and output is displayed on the console.
 *
 * Use this program to interactively test your assign6 modules.
 * You should not need to edit this program.
 */
#include "gpio.h"
#include "keyboard.h"
#include "console.h"
#include "shell.h"
#include "uart.h"

#define NUM_ROWS 30
#define NUM_COLS 80

void main(void) {
    gpio_init();
    uart_init();
    console_init(NUM_ROWS, NUM_COLS, GL_AMBER, GL_BLACK);
    keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);

    shell_init(keyboard_read_next, console_printf);
    shell_run();
}

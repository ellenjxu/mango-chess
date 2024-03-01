/* File: interrupts_console_shell.c
 * --------------------------------
 * This program runs the shell application. Input is read from the
 * the PS/2 keyboard and output is displayed on the console. Interrupts
 * are enabled to read key events.
 *
 * Use this program to interactively test your assign7 modules
 * and your complete system.
 * You should not need to edit this program.
 */
#include "console.h"
#include "interrupts.h"
#include "keyboard.h"
#include "shell.h"
#include "timer.h"
#include "uart.h"

void main(void) {
    interrupts_init();
    gpio_init();
    timer_init();
    uart_init();
    keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);
    console_init(30, 80, GL_GREEN, GL_BLACK);
    shell_init(keyboard_read_next, console_printf);

    interrupts_global_enable(); // everything fully initialized, now turn on interrupts
    shell_run();
}

/* File: uart_shell.c
 * ------------------
 * This program runs the shell application. Input is read from the
 * the PS/2 keyboard and output is printed to the uart.
 *
 * Use this program to interactively test your assign5 modules.
 * You should not need to edit this program.
 */
#include "uart.h"
#include "keyboard.h"
#include "printf.h"
#include "shell.h"

void main(void) {
    uart_init();
    keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);
    shell_init(keyboard_read_next, printf);

    shell_run();
}

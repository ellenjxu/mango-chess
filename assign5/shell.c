/* File: shell.c
 * -------------
 * ***** TODO: add your file header comment here *****
 */
#include "shell.h"
#include "shell_commands.h"
#include "uart.h"

#define LINE_LEN 80

static struct {
    input_fn_t shell_read;
    formatted_fn_t shell_printf;
} module;


// NOTE TO STUDENTS:
// Your shell commands output various information and respond to user
// error with helpful messages. The specific wording and format of
// these messages would not generally be of great importance, but
// in order to streamline grading, we ask that you aim to match the
// output of the reference version.
//
// The behavior of the shell commands is documented in "shell_commands.h"
// https://cs107e.github.io/header#shell_commands
// The header file gives example output and error messages for all
// commands of the reference shell. Please match this wording and format.
//
// Your graders thank you in advance for taking this care!


static const command_t commands[] = {
    {"help",  "help [cmd]",  "print command usage and description", cmd_help},
    {"echo",  "echo [args]", "print arguments", cmd_echo},
    {"clear", "clear",       "clear screen (if your terminal supports it)", cmd_clear},
};

int cmd_echo(int argc, const char *argv[]) {
    for (int i = 1; i < argc; ++i)
        module.shell_printf("%s ", argv[i]);
    module.shell_printf("\n");
    return 0;
}

int cmd_help(int argc, const char *argv[]) {
    /***** TODO: Your code goes here *****/
    (void)commands; // to quiet compiler warning before you have implemented this function
    return 0;
}

int cmd_clear(int argc, const char* argv[]) {
    //const char *ANSI_CLEAR = "\033[2J"; // if your terminal does not handle formfeed, can try this alternative?

    module.shell_printf("\f");   // minicom will correctly respond to formfeed character
    return 0;
}

void shell_init(input_fn_t read_fn, formatted_fn_t print_fn) {
    module.shell_read = read_fn;
    module.shell_printf = print_fn;
}

void shell_bell(void) {
    uart_putchar('\a');
}

void shell_readline(char buf[], size_t bufsize) {
    /***** TODO: Your code goes here *****/
}

int shell_evaluate(const char *line) {
    /***** TODO: Your code goes here *****/
    return 0;
}

void shell_run(void) {
    module.shell_printf("Welcome to the CS107E shell. Remember to type on your PS/2 keyboard!\n");
    while (1)
    {
        char line[LINE_LEN];

        module.shell_printf("Pi> ");
        shell_readline(line, sizeof(line));
        shell_evaluate(line);
    }
}

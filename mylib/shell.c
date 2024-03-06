/* File: shell.c
 * -------------
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 * Simple shell for Mango Pi.
 */
#include "shell.h"
#include "shell_commands.h"
#include "uart.h"
#include "strings.h"
#include "malloc.h"
#include "ps2_keys.h"

#define LINE_LEN 80
#define LINE_BUFSIZE (LINE_LEN + 1)
#define HISTORY_LEN 10

#define CACHE_SIZE 1024

#define BACKSPACE '\b'
#define HISTORY_PREFIX '!'

#define SIZE(arr) (sizeof(arr) / sizeof(*arr))

// Module-level global variables for shell
static struct {
    input_fn_t shell_read;
    formatted_fn_t shell_printf;
    char history[HISTORY_LEN][LINE_BUFSIZE];
    int history_size;
} module;

int cmd_history(int argc, const char **argv);
int cmd_hex(int argc, const char **argv);
int cmd_dec(int argc, const char **argv);
int cmd_calc(int argc, const char **argv);

static const command_t commands[] = {
    {"help",    "help [cmd]", "       print command usage and description", cmd_help},
    {"echo",    "echo [args]", "      print arguments", cmd_echo},
    {"clear",   "clear", "            clear screen (if your terminal supports it)", cmd_clear},
    {"reboot",  "reboot", "           reboot the Mango Pi", cmd_reboot},
    {"peek",    "peek [addr]", "      print contents of memory at address", cmd_peek},
    {"poke",    "poke [addr] [val]", "store value into memory at address", cmd_poke},
    {"history", "history", "          list previously executed commands", cmd_history},
    {"hex",     "hex [number]", "     convert number to hex", cmd_hex},
    {"dec",     "dec [number]", "     convert number to decimal", cmd_dec},
    {"calc",    "calc [ops...]", "    add, subtract, multiply, divide", cmd_calc},
};

extern void mango_reboot(void);

int cmd_echo(int argc, const char *argv[]) {
    for (int i = 0; i < argc; ++i)
        module.shell_printf("%s ", argv[i]);
    module.shell_printf("\n");
    return 0;
}

int cmd_help(int argc, const char *argv[]) {
    if (argc == 0) { // general 'help' command
        for (int i = 0; i < SIZE(commands); i++) {
            command_t command = commands[i];
            module.shell_printf("%s %s\n", command.usage, command.description);
        }
        return 0;
    }

    // find command in argument
    for (int i = 0; i < SIZE(commands); i++) {
        command_t command = commands[i];
        if (strcmp(argv[0], command.name) != 0)
            continue;

        // found it!
        module.shell_printf("%s %s\n", command.usage, command.description);
        return 0;
    }

    // command not found, error
    module.shell_printf("error: no such command '%s'\n", argv[0]);

    // error code
    return 1;
}

int cmd_clear(int argc, const char* argv[]) {
    module.shell_printf("\f");   // minicom will correctly respond to formfeed character
    return 0;
}

int cmd_reboot(int argc, const char **argv) {
    module.shell_printf("Rebooting...\n");
    mango_reboot();
    return 0; // will never return, but keep compiler happy
}

int cmd_peek(int argc, const char **argv) {
    if (argc < 1) { // no arguments
        module.shell_printf("error: peek expects 1 argument [addr]\n");
        return 1;
    }

    const char *resptr;
    
    // parse address
    unsigned long addr = strtonum(argv[0], &resptr);

    if (*resptr != '\0' || addr > UINTPTR_MAX) { // couldn't decode or too large
        module.shell_printf("error: peek cannot convert '%s'\n", argv[0]);
        return 1;
    }

    if (addr % 4 != 0) { // not 4-byte aligned
        module.shell_printf("error: peek address must be 4-byte aligned\n");
        return 1;
    }

    // cast value to pointer
    unsigned int *ptr = (unsigned int *)addr;

    // print value of address, success!
    module.shell_printf("%p: %08x\n", ptr, *ptr);

    return 0;
}

int cmd_poke(int argc, const char **argv) {
    if (argc < 2) { // not enough arguments
        module.shell_printf("error: poke expects 2 arguments [addr] and [val]\n");
        return 1;
    }

    const char *resptr1, *resptr2;

    // parse address and value
    unsigned long addr = strtonum(argv[0], &resptr1);
    unsigned long val = strtonum(argv[1], &resptr2);

    if (*resptr1 != '\0' || addr > UINTPTR_MAX) { // couldn't decode or too large
        module.shell_printf("error: poke cannot convert '%s'\n", argv[0]);
        return 1;
    }

    if (*resptr2 != '\0' || val >= 1L << 32) { // couldn't decode or too large
        module.shell_printf("error: poke cannot convert '%s'\n", argv[1]);
        return 1;
    }

    if (addr % 4 != 0) { // not 4-byte aligned
        module.shell_printf("error: poke address must be 4-byte aligned\n");
        return 1;
    }

    // cast address to pointer
    unsigned int *ptr = (unsigned int *)addr;

    // store given value, success!
    *ptr = (unsigned int) val;

    return 0;
}

/*
 * If index is nonnegative, returns the indexth command typed, if available.
 * If index is negative, returns the command typed index times ago. For example
 * if index = -1, it returns the latest command.
 * If unavailable, returns NULL.
*/
static char *get_history_element(int index) {
    if (index >= 0) {   // positive number, direct index
        if (index >= module.history_size || index < module.history_size - HISTORY_LEN)
            return NULL;

        return module.history[index % HISTORY_LEN];
    } else {            // negative number, index commands ago
        if (-index > module.history_size)
            return NULL;
        
        return get_history_element(module.history_size + index);
    }
}

int cmd_history(int argc, const char **argv) {
    // start from 10 commands ago and print the ones that exist
    for (int i = -HISTORY_LEN; i < 0; i++) {
        char *cmd = get_history_element(i);
        if (cmd == NULL) // command does not exist
            continue;
        
        module.shell_printf("  %d %s\n", module.history_size + i + 1, cmd);
    }
    return 0;
}

int cmd_hex(int argc, const char **argv) {
    if (argc == 0) {        // no arg, error
        module.shell_printf("error: hex expects 1 argument [number]\n");
        return 1;
    }

    const char *resptr;
    unsigned long number = strtonum(argv[0], &resptr);

    if (*resptr != '\0') {  // could not parse, error
        module.shell_printf("error: hex could not parse '%s'\n", argv[0]);
        return 1;
    }

    module.shell_printf("Hex: 0x%lx\n", number);
    return 0;
}

int cmd_dec(int argc, const char **argv) {
    if (argc == 0) {        // no arg, error
        module.shell_printf("error: dec expects 1 argument [number]\n");
        return 1;
    }

    const char *resptr;
    unsigned long number = strtonum(argv[0], &resptr);

    if (*resptr != '\0') {  // could not parse, error
        module.shell_printf("error: dec could not parse '%s'\n", argv[0]);
        return 1;
    }

    module.shell_printf("Dec: %ld\n", number);
    return 0;
}

int cmd_calc(int argc, const char **argv) {
    // index of argument we are looking at
    int argi = 0;
    const char *token = argv[argi];

    long accumulator = 0;
    char operator = '+';

    if (token[0] == '-') {
        operator = '-';
        token++;
    }

    // skip parse operator
    goto parse_operand;

    while (1) {
        // if this argument has run out, go to next one
        if (*token == '\0') {
            if (argi + 1 >= argc) // no more arguments, done
                break;
            else
                token = argv[++argi];
        }

        // operator is the first non-whitespace character
        operator = *token++;

        // if this argument has run out, go to next one
        if (*token == '\0') {
            if (argi + 1 >= argc)
                goto cannot_parse; // hanging operator, invalid input
            else
                token = argv[++argi];
        }

parse_operand:
        // parse operand
        long operand = strtonum(token, &token);

        switch (operator) {
        case '+':
            accumulator += operand;
            break;

        case '-':
            accumulator -= operand;
            break;

        case '/':
            accumulator /= operand;
            break;
        
        case 'x':
        case '*':
            accumulator *= operand;
            break;
        default:
            goto cannot_parse; // invalid operator
            break;
        }
    }

    module.shell_printf("Ans = %ld\n", accumulator);
    return 0;

cannot_parse:
    module.shell_printf("error: calc invalid input\n");
    return 1;
}

void shell_init(input_fn_t read_fn, formatted_fn_t print_fn) {
    module.shell_read = read_fn;
    module.shell_printf = print_fn;
    module.history_size = 0;
}

void shell_bell(void) {
    uart_putchar('\a');
}

/*
 * Given two strings, returns true if the first is a prefix of the second.
*/
static bool is_prefix(const char *prefix, const char *str) {
    while (*prefix != '\0' && *str != '\0') {
        if (*prefix != *str) return false;
        prefix++;
        str++;
    }
    return *prefix == '\0';
}

/*
 * Given a string, moves all characters one position to the right, leaving a
 * free position at the place of the pointer str. The resulting value of that
 * character is not guaranteed.
*/
static void scoot_right(char *str) {
    char next = *str++;
    do {
        char tmp = *str;
        *str = next;
        next = tmp;
    } while (*str++ != '\0');
}

/*
 * Given a string, moves all characters one position to the left, deleting the
 * first one (at position of pointer str).
*/
static void scoot_left(char *str) {
    while (*str != '\0') {
        *str = str[1];
        str++;
    }
}

/*
 * Prints characters in string buf to the shell, plus an aditional space (useful
 * to erase the last character on the screen after a deletion). Returns cursor
 * to initial position.
*/
static void print_buffer(char *buf) {
    module.shell_printf("%s ", buf);

    int length = strlen(buf) + 1; // 1 accounts for extra space

    char cache[CACHE_SIZE];
    memset(cache, BACKSPACE, length);
    cache[length] = '\0';
    module.shell_printf("%s", cache);
}

/*
 * Clear line in shell.
*/
static void clear_line(int cursor) {
    char cache[CACHE_SIZE];
    char *ptr = cache;

    // go all the way to the start of the line
    for (; cursor > 0; cursor--) {
        *ptr++ = BACKSPACE;
    }

    // write spaces (clearing the content)
    for (; cursor < LINE_LEN; cursor++) {
        *ptr++ = ' ';
    }

    // go all the way back again
    for (; cursor > 0; cursor--) {
        *ptr++ = BACKSPACE;
    }

    *ptr = '\0';

    module.shell_printf("%s", cache);
}

/*
 * Print prefix on the command line. This is used in two functions.
*/
static void print_command_line(void) {
    module.shell_printf("[%d] Pi> ", module.history_size + 1);
}

void shell_readline(char buf[], size_t bufsize) {
    // always have the last character be a null-terminator
    if (bufsize > 0) buf[0] = '\0';

    // saves current command after pressing up arrow
    char *saved_buf = NULL;

    // how many times the up arrow has been pressed (always negative or 0)
    int history_index = 0;

    // keep track of number of characters stored
    int buflen = 0;

    // keep track of cursor position
    int cursor = 0;

    while (1) {
        unsigned char ch = module.shell_read();

        if (ch == '\n') {                   // new line
            module.shell_printf("\n");
            if (buf[0] == HISTORY_PREFIX) {
                // need to handle commands with "!"
                // add new "[5] PI > " prefix
                print_command_line();

                // new command
                char *str;

                if (buf[1] == HISTORY_PREFIX) {
                    // simple !!, get latest command
                    str = get_history_element(-1);
                } else {
                    // using !prefix, get latest command with prefix

                    int i = 0; // start from latest
                    do {
                        str = get_history_element(--i);

                        if (is_prefix(buf + 1, str))
                            // found it!
                            break;
                    } while (str != NULL);
                }

                if (str == NULL) {
                    // no command found, simply clear
                    buf[0] = '\0';
                    buflen = 0;
                    cursor = 0;
                } else {
                    int len = strlen(str);

                    // write command to buffer
                    memcpy(buf, str, len + 1);
                    buflen = len;

                    // display command
                    module.shell_printf("%s", buf);
                }
            } else {
                // enter, done reading line
                free(saved_buf);
                return;
            }
        } else if (ch == '\t') {            // tab auto-complete
            if (cursor != buflen) // only allow tab when the cursor is at the end
                continue;

            // keep track of found command
            char *found = NULL;

            for (int i = 0; i < SIZE(commands); i++) {
                if (is_prefix(buf, commands[i].name)) {
                    if (found == NULL) {
                        // found one, must keep going in case there is another
                        found = (char *)commands[i].name;
                    } else {
                        // found two, ignore
                        found = NULL;
                        break;
                    }
                }
            }

            if (found == NULL) {
                // zero or multiple commands found, cannot autocomplete
                shell_bell();
                continue;
            }
            
            int len = strlen(found);

            if (len >= bufsize) // command would overflow buffer, ignore
                continue;
            
            // copy command to buffer and print
            memcpy(buf, found, len + 1);
            module.shell_printf("%s", found + cursor);
            buflen = len;
            cursor = len;

            // print space after tab (hacky, but DRY)
            ch = ' ';
            goto add_char;
        } else if (ch == PS2_KEY_ARROW_LEFT) {  // arrow left, move cursor
            if (cursor == 0) {
                // already at beginning
                shell_bell();
            } else {
                // move terminal cursor back one space
                module.shell_printf("%c", BACKSPACE);

                // update variable
                cursor--;
            }
        } else if (ch == PS2_KEY_ARROW_RIGHT) { // arrow right, move cursor
            if (cursor == buflen) {
                // already at end
                shell_bell();
            } else {
                // print the next character, gets to next position
                module.shell_printf("%c", buf[cursor]);

                // update variable
                cursor++;
            }
        } else if (ch == PS2_KEY_ARROW_UP) {    // arrow up, history command
            char *cmd = get_history_element(history_index - 1);

            if (cmd == NULL) {
                // no previous commands, ignore and bell
                shell_bell();
            } else {
                if (history_index == 0) {
                    // first time hitting up arrow, store command
                    saved_buf = malloc(buflen + 1);
                    memcpy(saved_buf, buf, buflen + 1);
                }

                // copy command to buffer
                buflen = strlen(cmd);
                memcpy(buf, cmd, buflen + 1);

                // clear line and print command
                clear_line(cursor);
                module.shell_printf("%s", buf);

                // update variables
                cursor = buflen;
                history_index--;
            }
        } else if (ch == PS2_KEY_ARROW_DOWN) {  // arrow down, history command
            if (history_index == 0) { // already at last command
                shell_bell();
                continue;
            }

            char *cmd = get_history_element(++history_index);

            // if now hitting current command
            if (history_index == 0) {
                // get saved command from the heap and free it
                buflen = strlen(saved_buf);
                memcpy(buf, saved_buf, buflen + 1);
                free(saved_buf);
            } else {
                // copy history command to buffer
                buflen = strlen(cmd);
                memcpy(buf, cmd, buflen + 1);
            }
            
            // clear line and print command
            clear_line(cursor);
            module.shell_printf("%s", buf);

            // update vairable
            cursor = buflen;
        } else if (ch == '\b') {       // backspace, attempt to delete
            if (cursor == 0) {
                // nothing to delete
                shell_bell();
            } else {
                // successful backspace
                cursor--;
                buflen--;
                scoot_left(buf + cursor);

                module.shell_printf("%c", BACKSPACE);
                print_buffer(buf + cursor);
            }

        } else {                // regular keystroke
            add_char:
            if (buflen + 1 >= bufsize) {
                // no space
                shell_bell();
            } else {
                // successful keystroke
                module.shell_printf("%c", ch);

                scoot_right(buf + cursor);
                buf[cursor] = ch;
                buflen++;
                cursor++;

                print_buffer(buf + cursor);
            }
        }
    }
}

/*
 * Returns 1 if ch is a space, tab, or newline. Otherwise, 0.
*/
static int is_whitespace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n';
}

/*
 * Returns pointer to the first non-whitespace character after
 * given pointer. "Whitespace" means a space, tab, or newline.
*/
static char *skip_whitespace(char *ptr) {
    while (is_whitespace(*ptr)) {
        ptr++; // no, this cannot be written as is_whitespace(*ptr++).
    }
    return ptr;
}

/*
 * Take a null-terminated string, an empty pointer array, and the size of
 * that array. Split string into tokens, each separated by an unknown amount
 * of whitespace (see is_whitespace above). Leading and trailing whitespace
 * is ignore. Pointers to each null-terminated token are stored in result.
 * Expect characters in input to be modified. Stops when resultsize tokens
 * have been found (typically, the client will provide a buffer more than
 * half the size of input, that way no tokens are lost).
 * 
 * Returns the number of pointers stored in result (never greater than
 * resultsize).
 * 
 * Implementation details: first, skip all initial whitespace. Then, continue
 * adding tokens to result and increasing the value of `tokens` until we hit
 * a null-terminator. There are several checks for null-terminators throughout
 * to account for all edge cases. Note that `tokens` stores the number of
 * tokens for which we have found the end at a given point.
*/
static int tokenize_arguments(char *input, char *result[], size_t resultsize) {
    // skip initial whitespace
    input = skip_whitespace(input);

    // edge case: empty or all-whitespace input
    if (*input == '\0') return 0;

    // number of tokens which we have found the *end* for
    int tokens = 0;

    // store address of first pointer
    result[0] = input;

    while (tokens < resultsize - 1) {
        if (is_whitespace(*input) || *input == '\0') {
            // we have found the end of a token
            tokens++;

            // if the end is a null-terminator, we are done
            if (*input == '\0')
                break;
            
            // set whitespace character to null-terminator to get a valid string
            *input++ = '\0';

            // skip whitespace (in case there are multiple spaces between tokens)
            input = skip_whitespace(input);

            // if after all the whitespace there is a null-terminator, we are done
            if (*input == '\0')
                break;
            
            // the beginning of a new token, keep track of it
            result[tokens] = input;
        }

        // look at next character
        input++;
    }

    return tokens;
}

/*
 * Updates module.history and module.history_size to store a given string
 * into history.
*/
static void save_command(const char *str) {
    memcpy(module.history[module.history_size % HISTORY_LEN], str, strlen(str) + 1);
    module.history_size++;
}

int shell_evaluate(const char *line) {
    if (*skip_whitespace((char *)line) != '\0')
        save_command(line);

    char *tokens[LINE_LEN / 2 + 1]; // worst case: half the characters are spaces

    // get tokens
    int tokenc = tokenize_arguments((char *)line, tokens, SIZE(tokens));

    // no tokens, error
    if (tokenc == 0)
        return -1;
    
    char *cmdname = tokens[0];

    // args are all strings after the command
    const char **argv = (const char **)tokens + 1;
    int argc = tokenc - 1;

    // test all commands and see if one works
    for (int i = 0; i < SIZE(commands); i++) {
        command_t command = commands[i];
        if (strcmp(command.name, cmdname) != 0)
            continue;
        
        // found one, call it!
        return command.fn(argc, argv);
    }

    // didn't find command, error
    module.shell_printf("error: no such command '%s'\n", cmdname);

    // error code
    return -1;
}

void shell_run(void) {
    module.shell_printf("Welcome to the CS107E shell. Remember to type on your PS/2 keyboard!\n");
    while (1)
    {
        char line[LINE_BUFSIZE];

        print_command_line();
        shell_readline(line, sizeof(line));
        shell_evaluate(line);
    }
}

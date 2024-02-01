/* File: print_pinout.c
 * --------------------
 * Sample test program that uses printf to output an ASCII version of
 * the Mango Pi pinout. (C translation of python $CS107E/bin/pinout.py)
 *
 * If your terminal does not support colors, change OUTPUT_ANSI_COLORS
 * from 1 to 0 to suppress color escapes and only print plain characters.
 *
 * Author: Julie Zelenski <zelenski@cs.stanford.edu>
 */

#include "printf.h"
#include "uart.h"

#define OUTPUT_ANSI_COLORS 1

#if OUTPUT_ANSI_COLORS
#define STRINGIFY_IMPL(x) #x
#define AS_STRING(x) STRINGIFY_IMPL(x)
#define ANSI_ESC(n) "\e[" AS_STRING(n) "m"
#else
#define ANSI_ESC(n) ""
#endif

#define BLACK   ANSI_ESC(40)
#define RED     ANSI_ESC(41)
#define GREEN   ANSI_ESC(42)
#define YELLOW  ANSI_ESC(43)
#define BLUE    ANSI_ESC(44)
#define MAGENTA ANSI_ESC(35)
#define WHITEFG ANSI_ESC(37)
#define NORMAL  ANSI_ESC(0)

static const char *board[] = {
    "    |OTG|  |USB|                | HDMI |     ",
    " O--|   |--|   |----------------| mini |---O ",
    " |                  +-------+      +-----+ | ",
    " |     " MAGENTA "Mango Pi" NORMAL "     |  D1   |      |micro| | ",
    " |      " MAGENTA "MQ-Pro" NORMAL "      |  SoC  |      | sd  | | ",
    " |                  +-------+      +-----+ | ",
    " |                                         | ",
    " | @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ 1 | ",
    " | - - - - - - - - - - - - - - - - - - - - | ",
    " | @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ | ",
    " O-----------------------------------------O ",
};

static struct pair_t {
    const char *label;
    const char *color;
} headers[20][2] = {
    { {"3V3",YELLOW},   {"5V",RED} },
    { {"PG13",GREEN},   {"5V",RED} },
    { {"PG12",GREEN},   {"GND",BLACK} },
    { {"PB7",GREEN},    {"PB8 (TX)",GREEN} },
    { {"GND",BLACK},     {"PB9 (RX)",GREEN} },
    { {"PD21",GREEN},   {"PB5",GREEN} },
    { {"PD22",GREEN},   {"GND",BLACK} },
    { {"PB0",GREEN},    {"PB1",GREEN} },
    { {"3V3",YELLOW},   {"PD14",GREEN} },
    { {"MOSI",GREEN},   {"GND",BLACK} },
    { {"MISO",GREEN},   {"PC1",GREEN} },
    { {"SCLK",GREEN},   {"CS0",GREEN} },
    { {"GND",BLACK},     {"PD15",GREEN} },
    { {"PE17",BLUE},    {"PE16",BLUE} },
    { {"PB10",GREEN},   {"GND",BLACK} },
    { {"PB11",GREEN},   {"PC0",GREEN} },
    { {"PB12",GREEN},   {"GND",BLACK} },
    { {"PB6",GREEN},    {"PB2",GREEN} },
    { {"PD17",GREEN},   {"PB3",GREEN} },
    { {"GND",BLACK},     {"PB4",GREEN} },
};

static void print_colored_header(const char *str, int row_index) {
    int pin_index = sizeof(headers)/sizeof(*headers) -1;
    for (int i = 0; str[i]; i++) {
        if (str[i] == '@') {
            printf("%s %s", headers[pin_index][row_index].color,  NORMAL);
            pin_index--;
        } else {
            printf("%c", str[i]);
        }
    }
    printf("\n");
}

static void print_board(void) {
    int header_index = 0;
    for (int i = 0; i < sizeof(board)/sizeof(*board); i++) {
        const char *rowstr = board[i];
        if (OUTPUT_ANSI_COLORS && rowstr[3] == '@') {
            print_colored_header(rowstr, header_index++);
        } else {
            printf("%s\n", rowstr);
        }
    }
}

static void print_header_table(void) {
    int pin_number = 1;
    for (int i = 0; i < sizeof(headers)/sizeof(*headers); i++) {
        struct pair_t *pair = headers[i];
        printf("  %s\t%s%s%02d%s|%s%s%02d%s  %s\n",
            pair[0].label,
            WHITEFG, pair[0].color, pin_number, NORMAL,
            WHITEFG, pair[1].color, pin_number+1, NORMAL,
            pair[1].label);
        pin_number += 2;
    }
}

void main(void) {
    uart_init();
    printf("\n");
    print_board();
    printf("\n");
    print_header_table();
}

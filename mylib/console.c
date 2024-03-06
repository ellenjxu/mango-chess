/* File: console.c
 * ---------------
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 * Console interface for Mango Pi.
 * Credit: Ben suggested using gl_draw_rect to clear the background of a row.
*/
#include "console.h"
#include "gl.h"
#include "malloc.h"
#include "printf.h"
#include "strings.h"
#include "timer.h"
#include <stdarg.h>

#define BUF_SIZE 2048

#define FRESH 0
#define STALE 1

#define FLICKER_DELAY_US 8000

#define LINE_SPACING 5

typedef char freshness_t;

// module-level variables, you may add/change this struct as you see fit!
static struct {
    color_t bg_color, fg_color;
    int line_height, line_width, nrows, ncols;
    int cursor_col, cursor_row;
    void *chars;        // 2-d row-major array
    freshness_t *row_freshness;
} module;

static void process_char(char ch);
static void draw_console(void);

void console_init(int nrows, int ncols, color_t foreground, color_t background) {
    // free previously allocatd memory (will be NULL if first call)
    free(module.chars);
    free(module.row_freshness);

    // initialize graphics library
    module.line_height = gl_get_char_height() + LINE_SPACING;
    module.line_width = gl_get_char_width() * ncols;
    gl_init(module.line_width, nrows * module.line_height - LINE_SPACING, GL_DOUBLEBUFFER);

    // save arguments
    module.fg_color = foreground;
    module.bg_color = background;
    module.nrows = nrows;
    module.ncols = ncols;

    // allocate memory for chars (+1 for null-terminator, see draw_console())
    module.chars = malloc(nrows * ncols + 1);

    // allocate row_freshness memory, one byte per row
    module.row_freshness = malloc(nrows);

    // clear console and set cursor
    console_clear();
}

void console_clear(void) {
    // set cursor
    module.cursor_col = 0;
    module.cursor_row = 0;

    // all characters are now clear (set to space)
    memset(module.chars, ' ', module.ncols * module.nrows);

    // all characters are freshly written
    memset(module.row_freshness, FRESH, module.nrows);

    // clear screen visually (both buffers)
    gl_clear(module.bg_color);
    gl_swap_buffer();
    gl_clear(module.bg_color);
}

int console_printf(const char *format, ...) {
    // NOTE: a lot of this code is directly copied from my printf.c
    // set up argument list
    va_list args;
    va_start(args, format);

    // set up buffer 
    char buf[BUF_SIZE];

    // call vsnprintf to do the heavy lifting
    int output = vsnprintf(buf, sizeof(buf), format, args);

    // clean up argument list
    va_end(args);

    // proces characters
    char *ptr = buf;
    while (*ptr != '\0')
        process_char(*ptr++);
    
    // paint to "draw buffer"
    draw_console();

    // display draw buffer to screen
    gl_swap_buffer();

    // if we start drawing too quickly, those changes are sometimes painted to
    // the screen, which causes an annoying flicker. we wait less than a ms to
    // avoid this.
    timer_delay_us(FLICKER_DELAY_US);

    // update new frame buffer to keep buffers in sync
    draw_console();

    // everything is fresh now
    memset(module.row_freshness, FRESH, module.nrows);

    // return number of characters written
    return output;
}

/*
 * Paints a rectangle of background color over row.
*/
static void paint_row_bg(int row) {
    if (row < 0 || row >= module.nrows) // invalid row
        return;

    gl_draw_rect(0, module.line_height * row, module.line_width, module.line_height, module.bg_color);
}

/*
 * Iterates over all rows and draws those that are stale to the draw buffer.
*/
static void draw_console(void) {
    char (*chars)[module.ncols] = module.chars;

    for (int row = 0; row < module.nrows; row++) {
        if (module.row_freshness[row] == FRESH)
            // row does not need updating
            continue;
        
        // paint background
        paint_row_bg(row);

        // set null-terminator at the end of the row such that we can call
        // gl_draw_string(). store the character that was there before in tmp
        const char tmp = chars[row + 1][0];
        chars[row + 1][0] = '\0'; // this never overflows, we allocated enough space

        gl_draw_string(0, module.line_height * row, chars[row], module.fg_color);

        // restore character we overwrote
        chars[row + 1][0] = tmp;
    }
}

/*
 * Sets all characters in a line to ' '.
*/
static void clear_line(int line) {
    if (line < 0 || line >= module.nrows)
        return;

    char (*chars)[module.ncols] = module.chars;
    memset(chars[line], ' ', module.ncols);
}

/*
 * Scoots every character in chars up by one row (discarding the top row).
 * Clears the bottom row. Sets all lines to stale.
*/
static void scroll_down() {
    char (*chars)[module.ncols] = module.chars;
    for (int row = 0; row < module.nrows - 1; row++) {
        for (int col = 0; col < module.ncols; col++) {
            chars[row][col] = chars[row + 1][col];
        }
    }
    clear_line(module.nrows - 1);
    memset(module.row_freshness, STALE, module.nrows);
}

/*
 * If cursor_col or cursor_row are out of bounds, makes necessary corrections.
 *
 *  - If cursor_col < 0, moves cursor to the last character of the previous
 *    line.
 *  - If cursor_col is too large, moves cursor to beginning of next line.
 *  - If cursor_row < 0, moves cursor to row 0 (this should never happen in
 *    practice).
 *  - If cursor_row is too large, scrolls down one line and sets the cursor_row
 *    to the bottom row. Does not modify cursor_col.
*/
static void correct_cursor() {
    if (module.cursor_col < 0) {
        module.cursor_col = module.ncols - 1;
        module.cursor_row--;
    } else if (module.cursor_col >= module.ncols) {
        module.cursor_col = 0;
        module.cursor_row++;
    }

    if (module.cursor_row < 0) {
        module.cursor_row = 0;
    } else if (module.cursor_row >= module.nrows) {
        scroll_down();
        module.cursor_row--;
    }
}

/*
 * Takes a character and modifies cursor_row, cursor_col and the values in chars
 * appropriately to handle it.
*/
static void process_char(char ch) {
    if (ch == '\r') {
        return; // ignore
    } else if (ch == '\n') {    // new line
        module.cursor_col = 0;
        module.cursor_row++;
        correct_cursor();
    } else if (ch == '\f') {    // clear console
        console_clear();
    } else if (ch == '\b') {    // backspace
        module.cursor_col--;
        correct_cursor();
    } else {                    // regular character   
        // correct cursor before, not after, becuse this matches the user's
        // mental model better
        correct_cursor();

        // set character
        char (*chars)[module.ncols] = module.chars;
        chars[module.cursor_row][module.cursor_col] = ch;

        // mark row as stale
        module.row_freshness[module.cursor_row] = STALE;
        
        // update cursor
        module.cursor_col++;
    }
}

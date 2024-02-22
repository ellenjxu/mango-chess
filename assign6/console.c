/* File: console.c
 * ---------------
 * ***** TODO: add your file header comment here *****
 */
#include "console.h"
#include "gl.h"

// module-level variables, you may add/change this struct as you see fit!
static struct {
    color_t bg_color, fg_color;
    int line_height;
} module;

void console_init(int nrows, int ncols, color_t foreground, color_t background) {
    // Please use this amount of space between console rows
    const static int LINE_SPACING = 5;

    module.line_height = gl_get_char_height() + LINE_SPACING;
    module.fg_color = foreground;
    module.bg_color = background;
    /***** TODO: Your code goes here *****/
}

void console_clear(void) {
    /***** TODO: Your code goes here *****/
}

int console_printf(const char *format, ...) {
    /***** TODO: Your code goes here *****/
	return 0;
}

static void process_char(char ch) {
    /***** TODO: Your code goes here *****/
    // Implementing this helper function is recommended
    // if ordinary char: insert ch into contents at current cursor position and
    // advance cursor
    // else if special char, do special handling
}

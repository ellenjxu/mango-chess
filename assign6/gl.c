/* File: gl.c
 * ----------
 * ***** TODO: add your file header comment here *****
 */
#include "gl.h"

void gl_init(int width, int height, gl_mode_t mode) {
    fb_init(width, height, mode);
}

int gl_get_width(void) {
    /***** TODO: Your code goes here *****/
    return 0;
}

int gl_get_height(void) {
    /***** TODO: Your code goes here *****/
    return 0;
}

color_t gl_color(unsigned char r, unsigned char g, unsigned char b) {
    /***** TODO: Your code goes here *****/
    return 0;
}

void gl_swap_buffer(void) {
    /***** TODO: Your code goes here *****/
}

void gl_clear(color_t c) {
    /***** TODO: Your code goes here *****/
}

void gl_draw_pixel(int x, int y, color_t c) {
    /***** TODO: Your code goes here *****/
}

color_t gl_read_pixel(int x, int y) {
    /***** TODO: Your code goes here *****/
    return 0;
}

void gl_draw_rect(int x, int y, int w, int h, color_t c) {
    /***** TODO: Your code goes here *****/
}

void gl_draw_char(int x, int y, char ch, color_t c) {
    /***** TODO: Your code goes here *****/
}

void gl_draw_string(int x, int y, const char* str, color_t c) {
    /***** TODO: Your code goes here *****/
}

int gl_get_char_height(void) {
    /***** TODO: Your code goes here *****/
    return 0;
}

int gl_get_char_width(void) {
    /***** TODO: Your code goes here *****/
    return 0;
}

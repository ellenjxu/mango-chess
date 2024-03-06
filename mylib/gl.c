/* File: gl.c
 * ----------
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 * Graphics library for Mango Pi.
 */
#include "gl.h"
#include "fb.h"
#include "font.h"
#include "printf.h"

#define ALPHA 0xFF000000

#define FIRST_GLYPH 0x21
#define LAST_GLYPH 0x7f

void gl_init(int width, int height, gl_mode_t mode) {
    fb_init(width, height, (fb_mode_t) mode);
}

int gl_get_width(void) {
    return fb_get_width();
}

int gl_get_height(void) {
    return fb_get_height();
}

color_t gl_color(unsigned char r, unsigned char g, unsigned char b) {
    color_t color = ALPHA;
    color |= b;
    color |= g << 8;
    color |= r << 16;
    return color;
}

void gl_swap_buffer(void) {
    fb_swap_buffer();
}

void gl_clear(const color_t c) {
    // cache constants
    const int width = gl_get_width();
    const int height = gl_get_height();

    unsigned int (*fb)[width] = fb_get_draw_buffer();

    // write to frame buffer
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            fb[y][x] = c;
        }
    }
}

void gl_draw_pixel(const int x, const int y, const color_t c) {
    // cache constants
    const int width = gl_get_width();
    const int height = gl_get_height();

    // if out of bounds, give up
    if (x >= width || y >= height || x < 0 || y < 0)
        return;

    unsigned int (*fb)[width] = fb_get_draw_buffer();

    // write to frame buffer
    fb[y][x] = c;
}

color_t gl_read_pixel(const int x, const int y) {
    // cache constants
    const int width = gl_get_width();
    const int height = gl_get_height();

    // if out of bounds, give up
    if (x >= width || y >= height || x < 0 || y < 0)
        return 0;

    unsigned int (*fb)[width] = fb_get_draw_buffer();

    // read from frame buffer
    return fb[y][x];
}

void gl_draw_rect(const int x, const int y, const int w, const int h, const color_t c) {
    // cache constants
    const int width = gl_get_width();
    const int height = gl_get_height();

    unsigned int (*fb)[width] = fb_get_draw_buffer();

    // clip bottom-right corner to screen
    const int endx = x + w < width  ? x + w : width;
    const int endy = y + h < height ? y + h : height;

    // clip top-left corner to screen
    const int x0 = x < 0 ? 0 : x;
    const int y0 = y < 0 ? 0 : y;

    // draw rect within bounds
    for (int dy = y0; dy < endy; dy++) {
        for (int dx = x0; dx < endx; dx++) {
            fb[dy][dx] = c;
        }
    }
}

void gl_draw_char(const int x, const int y, const char ch, const color_t c) {
    // cache constants
    const int width = gl_get_width();
    const int height = gl_get_height();

    // define glyph dimensions
    const int h = gl_get_char_height();
    const int w = gl_get_char_width();
    const int nbytes = w * h;

    // clip bottom-right corner to screen
    const int endx = x + w < width  ? x + w : width;
    const int endy = y + h < height ? y + h : height;

    // clip top-left corner to screen
    const int x0 = x < 0 ? 0 : x;
    const int y0 = y < 0 ? 0 : y;

    unsigned int (*fb)[width] = fb_get_draw_buffer();

    // get glyph from font
    unsigned char buf[nbytes];

    // get glyph. if character does not have a glyph, give up
    if (!font_get_glyph(ch, buf, nbytes))
        return;

    // parse glyph to 2-d array
    unsigned char (*glyph)[w] = &buf;

    // draw glyph within bounds
    for (int dy = y0; dy < endy; dy++) {
        for (int dx = x0; dx < endx; dx++) {
            if (glyph[dy - y][dx - x] == 0xFF) {
                fb[dy][dx] = c;
            }
            // if not 0xFF, do nothing (transparent)
        }
    }
}

void gl_draw_string(int x, int y, const char *str, color_t c) {
    // cache constants
    const int width = gl_get_width();
    const int w = gl_get_char_width();

    // call gl_draw_char for each character in string
    int dx = x;
    while (dx < width && *str != '\0') {
        gl_draw_char(dx, y, *str, c);

        str++;
        dx += w;
    }
}

int gl_get_char_height(void) {
    return font_get_glyph_height();
}

int gl_get_char_width(void) {
    return font_get_glyph_width();
}

static void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

// max distance we might draw a pixel away from the line
#define MAX_DIST 1.5F
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(a) ((a) < 0 ? -(a) : (a))
#define SQ(a) ((a) * (a))

static void draw_line_pixel(int x, int y, float Dsq, color_t c, bool xy_flipped) {
    if (Dsq > 3.0F) return;
    float Dsq_scaled = MIN(0.75F, Dsq / SQ(MAX_DIST));
    float intensity = 1.0F - Dsq_scaled;
    float red = (c & 0x00FF0000) >> 16;
    float green = (c & 0x0000FF00) >> 8;
    float blue = (c & 0x000000FF);
    red = red * intensity;
    green = green * intensity;
    blue = blue * intensity;
    c = (c & 0xFF000000) | ((int) red << 16) | ((int) green << 8) | (int) blue;
    if (xy_flipped)
        gl_draw_pixel(y, x, c);
    else
        gl_draw_pixel(x, y, c);

}

void gl_draw_line(int x1, int y1, int x2, int y2, color_t c) {
    bool xy_flipped = false;

    if (y1 == y2) {
        gl_draw_rect(x1, y1, ABS(x2 - x1), 1, c);
        return;
    } else if (x1 == x2) {
        gl_draw_rect(x1, y1, 1, ABS(y2 - y1), c);
        return;
    }

    if (ABS(y2 - y1) > ABS(x2 - x1)) {
        swap(&x1, &y1);
        swap(&x2, &y2);

        xy_flipped = true;
    }

    if (x2 < x1) {
        swap(&x1, &x2);
        swap(&y1, &y2);
    }

    float dx = ABS(x2 - x1);
    float dy = ABS(y2 - y1);

    int sign = 1;

    if (y2 < y1) {
        sign = -1;
    }

    int x = x1;
    int y = y1;

    const float m = dx / dy;
    const float b = y1 - m * x1;
    const float lengthsq = dx * dx + dy * dy;
    const float Dsq_denominator = 2.0f * lengthsq;

    float d = 2 * dy - dx;

    const int diffE  = 2 * dy;
    const int diffNE = 2 * (dy - dx);

    // draw endpoints (by definition, 0 distance)
    draw_line_pixel(x1, y1, 0, c, xy_flipped);
    draw_line_pixel(x2, y2, 0, c, xy_flipped);

    while (x++ < x2) {
        if (d <= 0) {   // choose E
            d += diffE;

            float v = m * x + b - sign*y;
            draw_line_pixel(x, y + 1, SQ(dx * (1 + v)) / lengthsq, c, xy_flipped);
            draw_line_pixel(x, y - 1, SQ(dx * (1 - v)) / lengthsq, c, xy_flipped);
        } else {        // choose NE
            d += diffNE;
            y += sign;

            float v = m * x + b - sign*y;
            draw_line_pixel(x, y + 1, SQ(dx * (1 - v)) / lengthsq, c, xy_flipped);
            draw_line_pixel(x, y - 1, SQ(dx * (1 + v)) / lengthsq, c, xy_flipped);
        }

        draw_line_pixel(x, y, SQ(d - dx) / Dsq_denominator, c, xy_flipped);
    }
}

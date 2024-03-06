/* File: fb.c
 * ----------
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 * Frame buffer for Mango Pi.
 */
#include "fb.h"
#include "de.h"
#include "hdmi.h"
#include "malloc.h"
#include "strings.h"

// module-level variables, you may add/change this struct as you see fit
static struct {
    int width;             // count of horizontal pixels
    int height;            // count of vertical pixels
    int depth;             // num bytes per pixel
    void *framebuffer;     // address of framebuffer memory
    void *drawbuffer;      // address of draw buffer (when in double-buffered mode)
    fb_mode_t mode;        // single or double buffer
} module;

void fb_init(int width, int height, fb_mode_t mode) {
    // free previously allocated memory (will be NULL if first call)
    free(module.framebuffer);
    free(module.drawbuffer);

    // save arguments
    module.width = width;
    module.height = height;
    module.depth = 4;
    module.mode = mode;

    // number of bytes in framebuffer
    int nbytes = module.width * module.height * module.depth;

    // allocate memory and clear to black
    module.framebuffer = malloc(nbytes);
    memset(module.framebuffer, 0x0, nbytes);

    if (mode == FB_DOUBLEBUFFER) {
        // allocate second buffer and clear to black
        module.drawbuffer = malloc(nbytes);
        memset(module.drawbuffer, 0x0, nbytes);
    } else {
        // single buffer mode; drawbuffer is undefined
        // set to NULL in case it was previously allocated, we don't want future
        // calls to fb_init to free it twice, which is undefined behavior
        module.drawbuffer = NULL;
    }

    // HDMI initialization magic
    hdmi_resolution_id_t id = hdmi_best_match(width, height);
    hdmi_init(id);
    de_init(width, height, hdmi_get_screen_width(), hdmi_get_screen_height());

    // set active framebuffer in display engine
    de_set_active_framebuffer(module.framebuffer);
}

int fb_get_width(void) {
    return module.width;
}

int fb_get_height(void) {
    return module.height;
}

int fb_get_depth(void) {
    return module.depth;
}

void *fb_get_draw_buffer(void){
    if (module.mode == FB_DOUBLEBUFFER) {
        return module.drawbuffer;
    } else {
        // in single-buffer mode, we always use framebuffer, and drawbuffer
        // is undefined
        return module.framebuffer;
    }
}

void fb_swap_buffer(void) {
    if (module.mode != FB_DOUBLEBUFFER) // single buffer; no-op
        return;
    
    // swap buffers
    void *tmp = module.framebuffer;
    module.framebuffer = module.drawbuffer;
    module.drawbuffer = tmp;

    // update active framebuffer in display engine
    de_set_active_framebuffer(module.framebuffer);
}

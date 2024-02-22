/* File: fb.c
 * ----------
 * ***** TODO: add your file header comment here *****
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
} module;

void fb_init(int width, int height, fb_mode_t mode) {
    /***** TODO: Make changes to support double-buffered mode  *****/

    module.width = width;
    module.height = height;
    module.depth = 4;
    int nbytes = module.width * module.height * module.depth;
    module.framebuffer = malloc(nbytes);
    memset(module.framebuffer, 0x0, nbytes);

    hdmi_resolution_id_t id = hdmi_best_match(width, height);
    hdmi_init(id);
    de_init(width, height, hdmi_get_screen_width(), hdmi_get_screen_height());
    de_set_active_framebuffer(module.framebuffer);
}

int fb_get_width(void) {
    /***** TODO: Your code goes here *****/
    return 0;
}

int fb_get_height(void) {
    /***** TODO: Your code goes here *****/
    return 0;
}

int fb_get_depth(void) {
    /***** TODO: Your code goes here *****/
    return 0;
}

void* fb_get_draw_buffer(void){
    /***** TODO: Your code goes here *****/
    return 0;
}

void fb_swap_buffer(void) {
    /***** TODO: Your code goes here *****/
}

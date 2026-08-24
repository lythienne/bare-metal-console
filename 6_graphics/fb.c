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
    void *drawbuffer;       // address of draw framebuffer memory (also active in single buffer)
    void *activebuffer;     // address of active framebuffer memory
    fb_mode_t mode;
} module;

void fb_init(int width, int height, fb_mode_t mode) {
    free(module.drawbuffer);
    free(module.activebuffer);

    module.mode = mode;
    module.width = width;
    module.height = height;
    module.depth = 4;
    int nbytes = module.width * module.height * module.depth;
    module.drawbuffer = malloc(nbytes);
    memset(module.drawbuffer, 0x0, nbytes);
    if (mode == FB_DOUBLEBUFFER) {
        module.activebuffer = malloc(nbytes);
        memset(module.activebuffer, 0x0, nbytes);
    }

    hdmi_resolution_id_t id = hdmi_best_match(width, height);
    hdmi_init(id);
    de_init(width, height, hdmi_get_screen_width(), hdmi_get_screen_height());
    de_set_active_framebuffer((mode == FB_DOUBLEBUFFER) ? module.activebuffer : module.drawbuffer);
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

void* fb_get_draw_buffer(void){
    return module.drawbuffer;
}

void fb_swap_buffer(void) {
    if (module.mode == FB_DOUBLEBUFFER) {
        de_set_active_framebuffer(module.drawbuffer);
        void *temp = module.drawbuffer;
        module.drawbuffer = module.activebuffer;
        module.activebuffer = temp;
    }
}

/* 
 * FILENAME: fb.c
 * ---------------------------------------------------------
 * Author: Auddithio Nag
 *
 * This code implments the framebuffer module needed for a 
 * graphics library processor. It initialises the framebuffer
 * using a single or double buffer mode (with swapping if 
 * needed) and also conatins important getter functions.
 */

#include "fb.h"
#include "assert.h"
#include "mailbox.h"

typedef struct {
    unsigned int width;       // width of the physical screen
    unsigned int height;      // height of the physical screen
    unsigned int virtual_width;  // width of the virtual framebuffer
    unsigned int virtual_height; // height of the virtual framebuffer
    unsigned int pitch;       // number of bytes per row
    unsigned int bit_depth;   // number of bits per pixel
    unsigned int x_offset;    // x of the upper left corner of the virtual fb
    unsigned int y_offset;    // y of the upper left corner of the virtual fb
    void *framebuffer;        // pointer to the start of the framebuffer
    unsigned int total_bytes; // total number of bytes in the framebuffer
} fb_config_t;

// fb is volatile because the GPU will write to it
static volatile fb_config_t fb __attribute__ ((aligned(16)));

void fb_init(unsigned int width, unsigned int height, unsigned int depth_in_bytes, fb_mode_t mode)
{
    fb.width = width;
    fb.virtual_width = width;
    fb.height = height;
    fb.virtual_height = (mode == FB_SINGLEBUFFER) ? height : 2 * height;
    fb.bit_depth = depth_in_bytes * 8; // convert number of bytes to number of bits
    fb.x_offset = 0;
    fb.y_offset = 0;

    // the manual states that we must set these value to 0
    // the GPU will return new values in its response
    fb.pitch = 0;
    fb.framebuffer = 0;
    fb.total_bytes = 0;

    // Send address of fb struct to the GPU as message
    bool mailbox_success = mailbox_request(MAILBOX_FRAMEBUFFER, (unsigned int)&fb);
    assert(mailbox_success); // confirm successful config
}


void fb_swap_buffer(void)
{
    // ignore if we're on single buffer mode
    if (fb.virtual_height == fb.height) return;

    // else swap
    fb.y_offset = (fb.y_offset == 0) ? fb.height : 0;
    mailbox_request(MAILBOX_FRAMEBUFFER, (unsigned int) &fb);
}

void* fb_get_draw_buffer(void)
{
    if (fb.virtual_height == fb.height || fb.y_offset == fb.height) {
        return fb.framebuffer;
    }
    return (void *)((unsigned int) fb.framebuffer + fb.pitch * fb.height); 
}

unsigned int fb_get_width(void)
{
    return fb.width;
}

unsigned int fb_get_height(void)
{
    return fb.height;
}

unsigned int fb_get_depth(void)
{
    return fb.bit_depth / 8;
}

unsigned int fb_get_pitch(void)
{
    return fb.pitch;
}


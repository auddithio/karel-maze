/*
 * FILNAME: gl.c
 * ----------------------------------------------------
 * Author: Auddithio Nag
 *
 * This module implements the graphic library. The graphics
 * display can be set to single and double buffer, and can 
 * display pixels, flat backgrounds, rectangular shapes and
 * even text.
 *
 * This also implements part of the extension, and draws an 
 * antialised line using the Xiaolin Wu line algorithm.
 */

#include "gl.h"
#include "strings.h"
#include "font.h"
#include "printf.h"
 

// format used is ARGB, with A (Opacity) as the most significant
const unsigned int OPACITY = 0xff << 24;
const unsigned int BLUE_SHIFT = 0;
const unsigned int GREEN_SHIFT = 8;
const unsigned int RED_SHIFT = 16;

// used for anti-aliasing in extension
static color_t background;

void gl_init(unsigned int width, unsigned int height, gl_mode_t mode)
{
    fb_init(width, height, 4, mode);    // use 32-bit depth always for graphics library
}

void gl_swap_buffer(void)
{
   fb_swap_buffer(); 
}

unsigned int gl_get_width(void)
{
    return fb_get_width(); 
}

unsigned int gl_get_height(void)
{
    return fb_get_height();
}

color_t gl_color(unsigned char r, unsigned char g, unsigned char b)
{
    return (b << BLUE_SHIFT) + (g << GREEN_SHIFT) 
            + (r << RED_SHIFT) + OPACITY;
}

void gl_clear(color_t c)
{
    unsigned int (*fb)[fb_get_pitch() / fb_get_depth()] = fb_get_draw_buffer();

    // fill color
    for (int i = 0; i < gl_get_height(); i++) {
        for (int j = 0; j < gl_get_width(); j++) {
            fb[i][j] = c;
        }
    } 
    background = c;
}

void gl_draw_pixel(int x, int y, color_t c)
{
    // bounds check
    if (x < 0 || x >= fb_get_width() || y < 0 || y >= fb_get_height()) {
        return;
    }

    unsigned int (*fb)[fb_get_pitch() / fb_get_depth()] = fb_get_draw_buffer();
    fb[y][x] = c;
}

color_t gl_read_pixel(int x, int y)
{
    // bounds check
    if (x < 0 || x >= gl_get_width() || y < 0 || y >= gl_get_height()) {
        return 0;
    }

    unsigned int (*fb)[fb_get_pitch() / fb_get_depth()] = fb_get_draw_buffer();
    return fb[y][x];
}

void gl_draw_rect(int x, int y, int w, int h, color_t c)
{
    // lower bound
    int min_x = x > 0 ? x : 0;
    int min_y = y > 0 ? y : 0;

    // upper bound
    int max_x = x + w <= gl_get_width() ? x + w : gl_get_width();
    int max_y = y + h <= gl_get_height() ? y + h : gl_get_height();

    unsigned int (*fb)[fb_get_pitch() / fb_get_depth()] = fb_get_draw_buffer();

    // draw
    for (int i = min_y; i < max_y; i++) {
        for (int j = min_x; j < max_x; j++) {
            fb[i][j] = c;
        }
    }
}

void gl_draw_char(int x, int y, char ch, color_t c)
{
    // get character glyph
    unsigned char buf[font_get_glyph_size()];
    if (!font_get_glyph(ch, buf, font_get_glyph_size())) {
        return;
    }
    
    // lower bound
    int min_x = x > 0 ? x : 0;
    int min_y = y > 0 ? y : 0;

    // upper bound
    int w = font_get_glyph_width();
    int h = font_get_glyph_height();
    int max_x = x + w <= (int) gl_get_width() ? x + w : gl_get_width();
    int max_y = y + h <= (int) gl_get_height() ? y + h : gl_get_height();

    unsigned int (*fb)[fb_get_pitch() / fb_get_depth()] = fb_get_draw_buffer();

    // draw
    int width = gl_get_char_width();
    for (int i = min_y; i < max_y; i++) {
        for (int j = min_x; j < max_x; j++) {

            // check if corresponding character in bitmap is 0xff or 0
            if (buf[width * (i - y) + (j - x)]) {
                fb[i][j] = c;
            }
        }
    }
}

void gl_draw_string(int x, int y, const char* str, color_t c)
{
    while (*str != '\0') {
        gl_draw_char(x, y, *str, c);

        // move forward
        x += gl_get_char_width();
        str++;
    }
}

unsigned int gl_get_char_height(void)
{
    return font_get_glyph_height();
}

unsigned int gl_get_char_width(void)
{
    return font_get_glyph_width();
}

/*
 * Finds the intermediate value between a channel of a colour 
 * of choice and the background colour. It does this for only 
 * one particular colour channel (must be specified). 
 * The returned value is a determined percentage away from the 
 * colour of choice's value at that channel.
 * 
 * @params  current color (color_t), channel shift (unsigned int), 
 *          percent (float)
 * @returns intermediate value (int)
 * @precon  channel shift must be of RED, BLUE or GREEN
 *          percent <= 1
 */
unsigned char blend_channel(color_t c1, int shift, float percent)
{
    // bitwise operations to get that colour channel
    int channel1 = (c1 >> shift) & 0xff;
    int channel2 = (background >> shift) & 0xff;

    return channel1 + (channel2 - channel1) * percent;
}

void gl_draw_line(int x1, int y1, int x2, int y2, color_t c) {
    float m = (y2 - y1) / (float) (x2 - x1); // gradient

    // determine ranges to draw the line
    int min_x = x1;
    int max_x = x2;
    float offset = y1 - m * x1;

    if (x1 > x2) {
        min_x = x2;
        max_x = x1;
    }

    // for each x value
    for (int x = min_x; x < max_x; x++) {

        // calculate y value
        float y = m * x + offset; // y = mx + c
        int truncated_y = (int) y;

        // calculate intensity of each pixel
        float intensity_upper = y - truncated_y;

        // isolate colours
        unsigned char blue = blend_channel(c, BLUE_SHIFT, intensity_upper);
        unsigned char green = blend_channel(c, GREEN_SHIFT, 
                                            intensity_upper);
        unsigned char red = blend_channel(c, RED_SHIFT, intensity_upper);

        color_t upper_color = gl_color(red, green, blue);
        color_t lower_color = background - upper_color + OPACITY;

        // plot points
        gl_draw_pixel(x, truncated_y, upper_color); // upper pixel
        gl_draw_pixel(x, truncated_y + 1, lower_color); // lower pixel
    }
}

color_t get_pixel_color(const unsigned char ref_img[], unsigned int counter) {
    unsigned char red = ref_img[counter]; 
    unsigned char green = ref_img[counter + 1]; 
    unsigned char blue = ref_img[counter + 2];
    color_t color = gl_color(red, green, blue);  
    return color; 
 
}

void gl_draw_image(const unsigned char ref_img[], int width, int height, int x, int y) {
    unsigned int counter = 0; 
    
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            color_t color = get_pixel_color(ref_img, counter); 
            if (color != GL_WHITE) {
                gl_draw_pixel(x + col, y + row, color);
            }
            counter += 3; 
        }
    }
} 


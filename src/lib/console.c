/*
 * FILNAME: console.c
 * ----------------------------------------------------
 * Author: Auddithio Nag
 *
 * This code implements the console using the graphics 
 * library. The user can type characters and strings
 * into the external monitor.
 *
 * The console also wraps the text around if it's a long 
 * line, and scrolls vertically down when it's full.
 */

#include "console.h"
#include "gl.h"
#include "malloc.h"
#include "printf.h"
#include <stdarg.h>
#include "strings.h"

// Please use this amount of space between console rows
const static int LINE_SPACING = 5;
static int line_height;

static color_t text_color;
static color_t bg_color;

static void *text;
static unsigned int numcols;
static unsigned int numrows;
    
// cursor position
unsigned int cur_y = 0;
unsigned int cur_x = 0;

static void process_char(char ch);

void console_init(unsigned int nrows, unsigned int ncols, color_t foreground, color_t background)
{
    line_height = gl_get_char_height() + LINE_SPACING;
    text_color = foreground;
    bg_color = background;
    numcols = ncols;
    numrows = nrows;

    // create console's 2D array
    text = malloc(nrows * ncols);
    memset(text, '\0', nrows * ncols);

    // cursor
    cur_y = 0;
    cur_x = 0;

    // set up
    gl_init(numcols * gl_get_char_width(), numrows * line_height, 
            GL_DOUBLEBUFFER);
    gl_clear(bg_color);
    gl_swap_buffer();
}

void console_clear(void)
{
    cur_x = 0;
    cur_y = 0;

    memset(text, '\0', numrows * numcols);
    gl_clear(bg_color);
    gl_swap_buffer();
    gl_clear(bg_color);
}

int console_printf(const char *format, ...)
{
    // set background
    gl_clear(bg_color);

    va_list args;
    va_start(args, *format);

    // format text
    char buf[numcols * numrows];
    vsnprintf(buf, numcols * numrows, format, args);

    // populate our 2D text array
    int count = 0;
    while (buf[count] != '\0') {
        process_char(buf[count++]); 
    }

    // configure console text
    char (*console)[numcols] = (char (*)[numcols]) text;

    // draw on screen
    for (int y = 0; y < numrows; y++) {
        for (int x = 0; x < numcols; x++) {

            // drawing char lets us wrap around without '\0'
            // and lets us backspace up a row
            gl_draw_char(x * gl_get_char_width(), y * line_height, 
                    console[y][x], text_color);            
        }
    }

    gl_swap_buffer();
	return count;
}

/* 
 * Thsi function implements the backspace ('\b')
 * functionality on the console module. It moves
 * the cursor and deletes the previous character.
 *
 * @params  none
 * @returns none
 */
void console_backspace(void) {
    char (*console)[numcols] = (char (*)[numcols]) text;

    // no backspace at start!
    if (cur_x == 0 && cur_y == 0) return;

    cur_x--;

    if (cur_x == -1) { // backspace to prev line
        cur_x = numcols - 1;
        cur_y--;
    }

    console[cur_y][cur_x] = '\0';
}

/* 
 * This function lets user "scroll down" the 2D text
 * array. It does this by shifting the rows up by one
 * (by copying) and making the last row empty.
 *
 * @params  none
 * @returns none
 */
void scroll_down(void) {
     
    char (*console)[numcols] = (char (*)[numcols]) text;

    // shift lines up by one
    int y = 0;
    while (y != numrows - 1) {

        // copy characters to prev row
        for (int x = 0; x < numcols; x++) {
            console[y][x] = console[y + 1][x];
        }
        y++;
    }
            
    // last line is empty
    memset(console[y], '\0', numcols);
    cur_y--;
}

static void process_char(char ch)
{   
    char (*console)[numcols] = (char (*)[numcols]) text;

    // insert special characters
    if (ch == '\b') {
        console_backspace();

    } else if (ch == '\n') {
        cur_x = 0;
        cur_y++;

    } else if (ch == '\f') {
        console_clear();

    // insert regular text
    } else { 
        console[cur_y][cur_x] = ch;
        cur_x++; 
    }

    // wrap around horizontally
    if (cur_x == numcols) {
        cur_y++; 
        cur_x = 0;
    }

    // if we're at end of screen
    if (cur_y == numrows) {
        scroll_down();
    }
}

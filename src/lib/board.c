/*
 * FILENAME: board.c
 * ------------------------------------------------
 * This code implements Karel's world, by drawing
 * and setting up the board in which Karel moves.
 *
 * The code is partly inspired by the grid.c code
 * we wrote in lab6.
 */

#include "board.h"
#include "gl.h"
#include "strings.h"
#include "printf.h"
#include "console.h"

#include "img/beeper.c"
#include "img/karelNorth.c"
#include "img/karelEast.c"
#include "img/karelWest.c" 
#include "img/karelSouth.c"

#include "img/julie_whitebg.c"
#include "img/patweb.c"

const unsigned int BOX_SIZE = 64;
const color_t BG_COLOR = GL_WHITE;
const color_t WALL_COLOR = GL_BLACK;
static board_config_t cur_board;
const unsigned int ASCII_10 = 48;

// keeps track of where the top left of the display is
struct point_t {
    int x;
    int y;
} top_left;

void board_init(const char *input_board[], int nrows, int display_dim) {

    // set up board
    cur_board = (board_config_t) {(char **)input_board, nrows, 
                strlen(input_board[0]), display_dim};

    // establish top left corner of displayed screen
    top_left.x = 0;
    top_left.y = nrows - display_dim;

    // set up screen
    gl_init(display_dim * BOX_SIZE, display_dim * BOX_SIZE, 
            GL_DOUBLEBUFFER);
}

/*
 * Draws the central plus in a box. Draws it in
 * the buffer function was called in.
 *
 * @params  x and y coordinate of upper left 
 *          corner of box
 * @returns none
 * @precon  x and y must be in range 
 */
void draw_central_plus(int x, int y) {

    int mid_x = x + BOX_SIZE / 2;
    int mid_y = y + BOX_SIZE / 2;

    // draw plus sign
    gl_draw_pixel(mid_x, mid_y, WALL_COLOR);
    gl_draw_pixel(mid_x, mid_y - 1, WALL_COLOR);
    gl_draw_pixel(mid_x - 1, mid_y, WALL_COLOR);
    gl_draw_pixel(mid_x + 1, mid_y, WALL_COLOR);
    gl_draw_pixel(mid_x, mid_y + 1, WALL_COLOR);
}

/*
 * Draws a vertical line on the board
 *
 * @params  (x, y) - starting coordinates of 
 *          the line, length of line (pixels)
 * @returns none
 */
void draw_vline(int x, int y, int length) {
    for (int h = y; h < y + length; h++) {
        gl_draw_pixel(x, h, WALL_COLOR);
    }
}

/*
 * Draws a horizontal line on the board
 *
 * @params  (x, y) - starting coordinates of 
 *          the line, length of line (pixels)
 * @returns none
 */
void draw_hline(int x, int y, int length) {
    for (int w = x; w < x + length; w++) {
        gl_draw_pixel(w, y, WALL_COLOR);
    }
}

/*
 * Scrolls the board in a particular given 
 * direction, provided there is more outside
 * the displayed dimensions
 *
 * @params  direction to be scrolled
 * @precon  only called when we are at edge
 */
void scroll(int x, int y) {
    if (x == top_left.x + cur_board.display_size) {
        top_left.x++;
    } else if (y < top_left.y) {
        top_left.y--;
    } else if (x < top_left.x) {
        top_left.x--;
    } else if (y == top_left.y + cur_board.display_size) {
        top_left.y++;
    }
}

void draw_start() {
  gl_clear(BG_COLOR); 

  unsigned int char_height = gl_get_char_height(); 
  gl_draw_string(0, 5, "  Welcome to", GL_BLACK);
  gl_draw_string(0, char_height + 10, "Karel's CS107E", GL_BLACK); 
  gl_draw_string(0, (char_height + 5) * 2 + 5, "  Adventure!", GL_BLACK); 

  unsigned int photo_height = (char_height + 5) * 3 + 10; 
  gl_draw_image(julie_whitebg.pixel_data, BOX_SIZE, BOX_SIZE, 0, photo_height); 
  gl_draw_image(karel_east.pixel_data, BOX_SIZE, BOX_SIZE, BOX_SIZE , photo_height); 
  gl_draw_image(pat_web.pixel_data, BOX_SIZE, BOX_SIZE, BOX_SIZE * 2, photo_height); 
  
  unsigned int start_height = photo_height + BOX_SIZE + 10; 
  gl_draw_string(0, start_height, " turn_left()", GL_BLACK); 
  gl_draw_string(0, start_height + char_height + 5, "  to start", GL_BLACK); 
   
  gl_swap_buffer(); 
}

void draw_rules() {
  gl_clear(BG_COLOR); 
  int char_height = gl_get_char_height(); 

  gl_draw_string(0, 5, "   Hi, Karel!", GL_BLACK); 
  gl_draw_string(0, char_height + 15, "Your job is to", GL_BLACK); 
  gl_draw_string(0, (char_height * 2) + 20, "find a beeper", GL_BLACK); 
  gl_draw_string(0, (char_height * 3) + 25, "in this world.", GL_BLACK); 

  gl_draw_string(0, (char_height * 4) + 35, "You can only", GL_BLACK); 
  gl_draw_string(0, (char_height * 5) + 40, "move() or ", GL_BLACK); 
  gl_draw_string(0, (char_height * 6) + 45, "turn_left().", GL_BLACK);

  gl_draw_string(0, (char_height * 7) + 55, "You got this!", GL_BLACK); 

  gl_swap_buffer(); 
}

void draw_resume(unsigned int time_taken) {
  gl_clear(BG_COLOR); 
  int char_height = gl_get_char_height();

  gl_draw_string(0, 20, " Time taken:", GL_BLACK);
  char buf[6];

  buf[0] = (char) ((time_taken / 600) + ASCII_10);
  printf("%c\n", buf[0]);
  buf[1] = (char) (((time_taken / 60) % 10) + ASCII_10);
  buf[2] = ':';
  buf[3] = (char) (((time_taken % 60) / 10) + ASCII_10);
  buf[4] = (char) (((time_taken % 60) % 10) + ASCII_10);
  buf[5] = '\0';
  
  gl_draw_string(64, char_height + 40, (const char *)buf, GL_BLACK);

  gl_draw_string(0, (char_height * 2) + 50, "Play again?", GL_BLACK); 
  gl_draw_string(0, (char_height * 3) + 60, "YES: turn left", GL_BLACK); 
  gl_draw_string(0, (char_height * 4) + 70, "NO: move", GL_BLACK); 
  
  gl_swap_buffer();  
}

void draw_end() {
  gl_clear(BG_COLOR); 
  int char_height = gl_get_char_height(); 

  gl_draw_string(0, 20, "  Thank you", GL_BLACK); 
  gl_draw_string(0, char_height + 40, "     for", GL_BLACK); 
  gl_draw_string(0, (char_height * 2) + 60, "   playing!", GL_BLACK);

  gl_swap_buffer(); 
}

void draw_board(int karel_x, int karel_y, int direction) {
    gl_clear(BG_COLOR);

    int size = cur_board.display_size;
    scroll(karel_x, karel_y);

    // draw basic board
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {

            char path = (cur_board.board)[y + top_left.y][x + top_left.x];

            draw_central_plus(x * BOX_SIZE, y * BOX_SIZE);

            if (path == SOUTH_WALL) { 
                draw_hline(BOX_SIZE * x, BOX_SIZE * (y + 1), BOX_SIZE);

            } else if (path == WEST_WALL) {
                draw_vline(x * BOX_SIZE, y * BOX_SIZE, BOX_SIZE);

            } else if (path == BEEPER) {
                gl_draw_image(beeper.pixel_data, BOX_SIZE, BOX_SIZE, 
                                x * BOX_SIZE, y * BOX_SIZE);

            } else if (path == PAT) {
                gl_draw_image(pat_web.pixel_data, BOX_SIZE, BOX_SIZE,
                                x * BOX_SIZE, y * BOX_SIZE);

            } else if (path == JULIE) {
                gl_draw_image(julie_whitebg.pixel_data, BOX_SIZE, BOX_SIZE,
                                x * BOX_SIZE, y * BOX_SIZE);
            }
        }
    }

    // draw karel
    karel_x = (karel_x - top_left.x) * BOX_SIZE;
    karel_y = (karel_y - top_left.y) * BOX_SIZE;
    if (direction == EAST) 
        gl_draw_image(karel_east.pixel_data, BOX_SIZE, BOX_SIZE, karel_x, karel_y); 
    if (direction == NORTH) 
        gl_draw_image(karel_north.pixel_data, BOX_SIZE, BOX_SIZE, karel_x, karel_y); 
    if (direction == WEST) 
        gl_draw_image(karel_west.pixel_data, BOX_SIZE, BOX_SIZE, karel_x, karel_y); 
    if (direction == SOUTH) 
        gl_draw_image(karel_south.pixel_data, BOX_SIZE, BOX_SIZE, karel_x, karel_y); 

    gl_swap_buffer();
}


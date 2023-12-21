#ifndef BOARD_H
#define BOARD_H

/*
 * FILENAME: board.h
 * ---------------------------------------------
 * Implements the world for our beloved Karel from
 * CS106A. We draw the board to use for Karel,
 * inspired by the grid.c file from lab6.
 */

/* 
 * Defines a board struct to use throughout the
 * code.
 */
typedef struct board_config {
    char **board; // pointer to board
    int num_rows; // number of rows in board
    int num_cols; // number of columns in board
    int display_size; // dimension of square board to be displayed
} board_config_t;

// convention used to encode walls
enum {
    FREE = '-',
    SOUTH_WALL = 's',
    WEST_WALL = 'w',
    BEEPER = 'b',
    PAT = 'p',
    JULIE = 'z',
};

// directions
enum {EAST, NORTH, WEST, SOUTH} directions;

/* 
 * 'board_init'
 *
 * Initialises the board 
 *
 * @params  board, number of rows in board, and length of square
 *          display on console
 * @returns none
 * @precon  board must be rectangular
 */
void board_init(const char *input_board[], int nrows, int display_dim);

/* 
 * 'draw_start'
 *
 * This function draws the start screen of the game
 *
 * @params  none
 * @returns none
 */
void draw_start();

void draw_rules();

void draw_resume();

void draw_end(); 

/*
 * 'draw_board'
 *
 * Reads a given board and draws out the board on 
 * the graphics console, including Karel.
 *
 * The board to read from is implemented as a 2D 
 * array of characters (look in board.c to see 
 * convention used to encode Karel's world).
 *
 * @params  Karel's x position, Karel's y position
 *          direction Karel is facing
 * @returns none
 * @precon  strings in board must be of equal length
 */
void draw_board(int karel_x, int karel_y, int direction);

#endif


/*
 * FILENAME: karel_world.c
 * -----------------------------------------
 * Implements Karel's beloved world in a 2D 
 * maze
 */

#include "karel_world.h"
#include "board.h"
#include "accel.h"
#include "strings.h"
#include "timer.h"
#include "printf.h"

const unsigned int DELAY_MS = 250;

// the world we want to implement
#define NUM_ROWS 10
const char *board[NUM_ROWS] = 
{
    /*
    "---",
    "---",
    "---",
    */
    "-sssw--ws-",
    "bw---sssws",
    "--sw---psw",
    "-w------s-",
    "s-sw--w-w-",
    "-ww---w-zw",
    "-w---sw---",
    "-w-w-sswss",
    "--sw-ss-s-",
    "--w-----w-",

};

// set karel's starting position and direction
static pos_t karel;
static pos_t beeper = (pos_t) {-10, -10, -10}; // no beeper by default

const unsigned int DISPLAY_DIM = 3;

void karel_world_init() {
    accel_init();
    board_init(board, NUM_ROWS, DISPLAY_DIM);

    // set karel's starting position and direction
    karel = (pos_t) {0, NUM_ROWS - 1, EAST};

    // check if we have beeper
    int width = strlen(board[0]);
    for (int y = 0; y < NUM_ROWS; y++) {
        for (int x = 0; x < width; x++) {
            if (board[y][x] == BEEPER) {
                beeper.x = x;
                beeper.y = y;
            }
        }
    }

    printf("Start!\n");
    draw_board(karel.x, karel.y, karel.dir);
}

/*
 * Checks if Karel is within the bounds of the maze
 * Returns 1 if it is, 0 if not.
 *
 * @precon  must be called when the move is forward
 */
int is_move_valid(int x, int y) {
           // within cols
    return x >= 0 && x < strlen(board[0]) 

            // within rows
            && y >= 0 && y < NUM_ROWS 

            // doesn't collide with west wall
            && !(board[y][x] == WEST_WALL && karel.dir == EAST)
            && !(board[karel.y][karel.x] == WEST_WALL && karel.dir == WEST)

            // doesn't collide with east wall
            && !(board[y][x] == SOUTH_WALL && karel.dir == NORTH)
            && !(board[karel.y][karel.x] == SOUTH_WALL 
                    && karel.dir == SOUTH);

}

int update_karel_world() {
    int move = accel_read_move();
    pos_t next_move = karel;

    // check moves
    if (move == MOVE_FORWARD) {

        if (karel.dir == EAST) {
            next_move.x++; // no bounds or wall checking!
        } else if (karel.dir == SOUTH) {
            next_move.y++;
        } else if (karel.dir == WEST) {
            next_move.x--;
        } else if (karel.dir == NORTH) {
            next_move.y--;
        }

        if (!is_move_valid(next_move.x, next_move.y)) {
            printf("\a"); // shell bell!
            timer_delay_ms(DELAY_MS);
            return 0;
        }

        karel = next_move; // update karel

    } else if (move == TURN_LEFT) {
        karel.dir = (karel.dir + 1) % 4;
    }
    
    draw_board(karel.x, karel.y, karel.dir);
    timer_delay_ms(DELAY_MS);

    // check if game is over
    if (karel.x == beeper.x && karel.y == beeper.y) {
        return 1;
    }

    return 0;
}

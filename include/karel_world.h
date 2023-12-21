#ifndef KAREL_WORLD_H
#define KAREL_WORLD_H

/*
 * FILENAME: karel_world.h
 * -------------------------------------------------
 * Implements the game engine of the game of Karel.
 */

// position and direction of Karel
typedef struct pos {
    int x;
    int y;
    int dir;
} pos_t;

/*
 * "karel_world_init"
 *
 * Initialises the console display of Karel's world
 */
void karel_world_init(void);

/*
 * "update_karel_world"
 *
 * Updates karel world according to the movements of 
 * the player. Returns true (1) when Karel finds the 
 * beeper and game is over, false otherwise.
 *
 * @params  none
 * @returns 1 (if game is over), 0 (game not over)
 */
int update_karel_world(void);

#endif

#ifndef ACCEL_H
#define ACCEL_H

/*
 * FILNAME: accel.h
 * -------------------------------------------------
 * This file initialises the data from the LSM6DS33
 * accelerometer/gyroscope. It packages them into 
 * specific events the user can incorporate.
 *
 * Partly inspired by the keyboard.h module we had
 * implemented in assign5
 */

// The moves we can register
enum moves {
    MOVE_FORWARD,
    TURN_LEFT,
    MOVE_BACKWARD,
    TURN_RIGHT,
    MOVE_RIGHT,
    MOVE_LEFT,
};

/*
 * 'accel_init'
 *
 * Initialises the accelerometer to be used.
 *
 * @params  none
 * @returns none
 */
void accel_init(void);

/*
 * 'accel_read_move'
 *
 * Waits until a move is registered. If a move is
 * valid, returns that move's value. Otherwise, it
 * raises an alarm.
 *
 * @params  none
 * @returns valid move (int)
 */
int accel_read_move(void);

#endif

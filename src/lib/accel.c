/*
 * FILENAME: accel.c
 * ------------------------------------------------
 * Implements the functions needed to read the data
 * from the LSM6DS33 accelerometer and generate moves
 *
 * Inspired by the keyboard.c code we implemented in
 * assign5.
 */

#include "accel.h"
#include "i2c.h"
#include "LSM6DS33.h"
#include "assert.h"

const unsigned int LSM6DS33 = 0x69;
const signed int ACC_THRESHOLD_Z = 600; // in mg
const signed int GYR_THRESHOLD_Z = 1000; // in degrees

void accel_init() {
    i2c_init();
    lsm6ds33_init();
    assert(lsm6ds33_get_whoami() == LSM6DS33); // should be 69

    lsm6ds33_enable_accelerometer();
    lsm6ds33_enable_gyroscope();
}

int accel_read_move() {

    // accelerometer and gyroscope
    short xa, ya, za, xg, yg, zg;

    lsm6ds33_read_accelerometer(&xa, &ya, &za); 
    lsm6ds33_read_gyroscope(&xg, &yg, &zg);

    while (1) { // implements the delay

        if (zg / 16 > GYR_THRESHOLD_Z) {
            return TURN_LEFT; 
        }

        if (za / 16 < ACC_THRESHOLD_Z) {
            return MOVE_FORWARD;
        }


        lsm6ds33_read_accelerometer(&xa, &ya, &za);
        lsm6ds33_read_gyroscope(&xg, &yg, &zg);
    }
}


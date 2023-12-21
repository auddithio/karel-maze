#include "printf.h"
#include "uart.h"
#include "timer.h"
#include "board.h"
#include "accel.h"
#include "gl.h"
#include "karel_world.h"

void game_init() {
    karel_world_init(); 
    timer_init(); 
}

void play_game() {
    while (!update_karel_world()){}; 
}

void karel_adventure(void) {
    while (1) {

        game_init(); 
    
        // 1. Display Start Screen
        int move = MOVE_FORWARD; 
        while (move != TURN_LEFT) {
            draw_start(); 
            move = accel_read_move(); 
        }

        // 2. Display Rules 
        draw_rules(); 
        timer_delay(4); 

        unsigned int start = timer_get_ticks();

        // 3. Play Game
        play_game(); 

        unsigned int time_taken_s = (timer_get_ticks() - start) / 1000000;

        // 4. Resume Screen 
        draw_resume(time_taken_s);
        timer_delay(4);
        move = accel_read_move(); 

        if (move == MOVE_FORWARD) {
            break; 
        }
    }

    draw_end();
    timer_delay(4);
}


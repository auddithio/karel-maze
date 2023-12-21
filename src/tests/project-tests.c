#include "printf.h"
#include "uart.h"
#include "board.h"
#include "accel.h"
#include "timer.h"
#include "gl.h"
#include "karel_world.h"
#include "game.h"

/* 
 * Tests basic board
 */
void test_board(void) {

    // an empty 3x3 board
    const char *board[3] = 
    {
        "---",
        "---",
        "---",
    };

    board_init(board, 3, 3);

    draw_start();
    timer_delay(2); 
    draw_rules();
    timer_delay(2); 
    draw_resume(660);
    timer_delay(2); 
    draw_end(); 
    
    timer_delay(5); 
    draw_board(0, 0, 0);
    timer_delay(1); 
    draw_board(1, 0, 0); // 1 step east
    timer_delay(1); 
    draw_board(1, 0, 1); // turn left
    timer_delay(1); 
    draw_board(1, 0, 2); // turn left again
    timer_delay(1); 
    draw_board(0, 0, 2); // go back 1 step
    
}

void test_complex_board(void) {

    const char *board[3] = 
    {
        "-s-",
        "-w-",
        "--w",
    };

    board_init(board, 3, 3);
    draw_board(0, 0, 0);
}

void test_accel_gyro(void) {

    accel_init();
    while (1) {

        int move = accel_read_move();

        if (move == MOVE_FORWARD) {
            printf("move forward\n");
            timer_delay_ms(1000);
        } else if (move == TURN_LEFT) {
            printf("turn left\n");
            timer_delay_ms(1000);
        }
    }
}

void test_karel_world(void) {
    karel_world_init();
    while (1) {
        update_karel_world();
    }
}

void test_game() {
    karel_adventure(); 
}

void main(void)
{
    uart_init();
    timer_init();
    test_board();
    test_complex_board();
   
    test_accel_gyro();
    test_karel_world();
    test_game(); 
    printf("Running tests from file %s\n", __FILE__);
    uart_putchar(EOT);
}

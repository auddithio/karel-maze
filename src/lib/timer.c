/* 
 * TIMER LIBRARY
 * Name: Auddithio Nag
 *
 * This code implements the timer program
 * based on the internal ARM tick counter
 */
#include "timer.h"
volatile unsigned int *CLO = (void *)0x20003004;

void timer_init(void) {
}

/* This function returns the value of the tick counter
 * of the ARM itself
 */
unsigned int timer_get_ticks(void) {
    unsigned int time = *CLO;
    return time;
}

void timer_delay_us(unsigned int usecs) {
    unsigned int start = timer_get_ticks();
    while (timer_get_ticks() - start < usecs) { /* spin */ }
}

void timer_delay_ms(unsigned int msecs) {
    timer_delay_us(1000*msecs);
}

void timer_delay(unsigned int secs) {
    timer_delay_us(1000000*secs);
}

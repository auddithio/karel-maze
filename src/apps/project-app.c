#include "printf.h"
#include "uart.h"
#include "i2c.h"
#include "timer.h"

#include "LSM6DS33.h"
#include "board.h"
#include "game.h"


void main(void) {

    uart_init();
    karel_adventure();
    uart_putchar(EOT);
}

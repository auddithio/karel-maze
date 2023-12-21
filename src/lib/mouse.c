#include "gpio.h"
#include "ps2.h"
#include "mouse.h"

// This module only needs to be implemented if you are doing the extension
// Also be sure to add mouse.o to ALL_LIBPI_MODULES in the assign7.makefile so
// this module is included in your libmypi.a

#define CMD_RESET 0xFF
#define CMD_ENABLE_DATA_REPORTING 0xF4

static ps2_device_t *dev;

void mouse_init(unsigned int clock_gpio, unsigned int data_gpio) {
    dev = ps2_new(clock_gpio, data_gpio);

    // TODO: Your code here
}

mouse_event_t mouse_read_event(void)
{
    mouse_event_t evt = { 0 };

    // TODO: Your code here

    return evt;
}


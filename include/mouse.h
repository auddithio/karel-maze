#ifndef MOUSE_H
#define MOUSE_H

#include "gpio.h"
#include <stdbool.h>


typedef enum {
    MOUSE_BUTTON_PRESS = 0,
    MOUSE_BUTTON_RELEASE = 1,
    MOUSE_MOVED = 2,
    MOUSE_DRAGGED = 3,
} mouse_action_t;


typedef struct {
    mouse_action_t action;
    int dx, dy;
    bool x_overflow, y_overflow;
    bool left, middle, right;
} mouse_event_t;

#define MOUSE_CLOCK GPIO_PIN5
#define MOUSE_DATA GPIO_PIN6

/*
 * `mouse_init`: Required initialization for mouse.
 *
 * The mouse must first be initialized before any mouse events can be read.
 * The two arguments are the GPIO pins of the PS2 clock and data lines
 * to use when creating the PS2 device.
 *
 * @param clock_gpio    the gpio connected to the clock line of mouse
 * @param data_gpio     the gpio connected to the data line of mouse
 */
void mouse_init(unsigned int clock_gpio, unsigned int data_gpio);

/*
 * `mouse_read_event`
 *
 * The function reads (blocking) the next event from the mouse.
 * Returns a `mouse_event_t` struct that represents the mouse event.  The
 * struct includes the relative change in x and y (dx, dy fields). A delta
 * value will be in range +/- 255, the x_overflow/y_overflow fields are true
 * if actual value of dx/dy exceeded +/- 255.  The left/middle/right fields
 * give the state of the mouse buttons. Each field is true if the corresponding
 * mouse button is down, false otherwise.
 *
 * @return         mouse_event_t struct containing event data
 */
mouse_event_t mouse_read_event(void);

#endif

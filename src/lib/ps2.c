/*
 * FILE: ps2.c
 * --------------------------------------------------
 *  Author: Auddithio Nag
 *
 *  This code reads the scancodes sent by a PS2 device
 *  to one of our GPIO pins.
 *
 *  It also checks the error and parity bits of a 
 *  transmitted scancode to verify the transmission is
 *  correct.
 */

#include "gpio.h"
#include "gpio_extra.h"
#include "malloc.h"
#include "ps2.h"
#include "gpio_interrupts.h"
#include "uart.h"
#include "ringbuffer.h"
#include "printf.h"

// This struct definition fully defines the type declared in ps2.h
// Since the ps2.h API only uses this struct as a pointer, its
// definition can be implementation-specific and deferred until
// here.
struct ps2_device {
    unsigned int clock; // GPIO pin number of the clock line
    unsigned int data;  // GPIO pin number of the data line
    unsigned int scancode;
    unsigned int scancode_len;
    unsigned int scancode_sum;
    rb_t *queue;
};

/* This function waits till the the falling edge of the clock, to 
 * start recording the new bit
 *
 * @params  the ps2 device (ps2_device_t *)
 * @returns data read from ps2 device
 */
unsigned int read_bit(ps2_device_t *dev) {
    while (gpio_read(dev->clock) == 0) {}
    while (gpio_read(dev->clock) == 1) {}
    return gpio_read(dev->data);
}

/*
 * Handler function to process the bits that arrive
 * when a key is pressed. It fills out the queue
 * in dev with the processed scancodes.
 *
 * @params  program counter (unsigned int), 
 *          pointer to ps2_device_t (passed as void *)
 * @returns none
 */
void key_press(unsigned int pc, void *aux_data) {

    // read data
    ps2_device_t *dev = (ps2_device_t *)aux_data;
    unsigned int data = gpio_read(dev->data);

    // start bit (should be 0)
    if (dev->scancode_len == 0) {
        if (data == 0) dev->scancode_len++;

    // parity bit
    } else if (dev->scancode_len == 9) { 

        if ((dev->scancode_sum + data) % 2 == 1) { // odd parity
            dev->scancode_len++;
        } else { // else restart
            dev->scancode = 0;
            dev->scancode_len = 0;
        }

    // stop bit (should be 1)
    } else if (dev->scancode_len == 10) {
        if (data) { // add to queue if valid
            rb_enqueue(dev->queue, dev->scancode);
        } 

        // restart for next scancode
        dev->scancode = 0;
        dev->scancode_len = 0;
        dev->scancode_sum = 0;

    // the scancode bits, LSB first
    } else {
        dev->scancode += data << (dev->scancode_len - 1);
        dev->scancode_sum += data;
        dev->scancode_len++;
    }

    gpio_clear_event(dev->clock);
}

ps2_device_t *ps2_new(unsigned int clock_gpio, unsigned int data_gpio)
{
    // consider why must malloc be used to allocate device
    ps2_device_t *dev = malloc(sizeof(*dev));

    dev->clock = clock_gpio;
    gpio_set_input(dev->clock);
    gpio_set_pullup(dev->clock);

    dev->data = data_gpio;
    gpio_set_input(dev->data);
    gpio_set_pullup(dev->data);

    // set up 
    dev->scancode = 0;
    dev->scancode_len = 0;
    dev->scancode_sum = 0;
    dev->queue = rb_new();

    // configure interrupts
    gpio_interrupts_init();
    gpio_enable_event_detection(clock_gpio, GPIO_DETECT_FALLING_EDGE);
    gpio_interrupts_register_handler(clock_gpio, key_press, dev);
    gpio_interrupts_enable();

    return dev;
}

unsigned char ps2_read(ps2_device_t *dev)
{
    // wait till we have a scancode
    while (rb_empty(dev->queue)) {};

    unsigned int queued_data = 0;
    rb_dequeue(dev->queue, (int *) &queued_data);      
    return queued_data;
}

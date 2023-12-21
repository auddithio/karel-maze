/*
 * GPIO LIBRARY
 * Name: Auddithio Nag
 *
 * This code implements the GPIO modules for the pins.
 * This include setting a function to a pin (including
 * input/output) and reading/writing from a pin
 */
 #include "gpio.h"

// shortcut to use the memory addresses of common registers
struct gpio {
    unsigned int fsel[6];
    unsigned int reservedA;
    unsigned int set[2];
    unsigned int reservedB;
    unsigned int clr[2];
    unsigned int reservedC;
    unsigned int lev[2];
};

volatile struct gpio *gpio = (struct gpio *)0x20200000;

void gpio_init(void) {
    // no initialization required for this peripheral
}

/* This function sets a function to a pin
 */
void gpio_set_function(unsigned int pin, unsigned int function) {

    if (function < GPIO_FUNC_INPUT || function > GPIO_FUNC_ALT3) {
        return;
    }

    // configure correct FSEL
    unsigned int fsel_num = pin / 10;
    
    //configure correct bit seq
    unsigned int pin_num = pin % 10;

    // each 3-bit func starts at 
    unsigned int complement = ~(7 << (3 * pin_num));

    // presever current FSEL value by and-ing it
    gpio->fsel[fsel_num] &= complement;

    // or it with the value we want
    gpio->fsel[fsel_num] |= (function << (3 * pin_num));
}

/* This function gets the function of a pin
 */
unsigned int gpio_get_function(unsigned int pin) {
    
    // error checking
    if (pin < GPIO_PIN_FIRST || pin > GPIO_PIN_LAST) {
        return GPIO_INVALID_REQUEST;
    }
    
    // configure FSEL and pin number
    unsigned int fsel_num = pin / 10;
    unsigned int pin_num = pin % 10;

    // get that number
    unsigned int function = gpio->fsel[fsel_num] >> (3 * pin_num);
    function &= 7;

    return function;
}

/* This function sets a pin as an input
 */
void gpio_set_input(unsigned int pin) {
    gpio_set_function(pin, GPIO_FUNC_INPUT);
}

/* This function sets a pin as an output
 */
void gpio_set_output(unsigned int pin) {
    gpio_set_function(pin, GPIO_FUNC_OUTPUT);
}

/* This function either writes a high state (by writing 
 * 1 to a SET register) or a low state (by writing 1 to a
 * CLR register)
 */
void gpio_write(unsigned int pin, unsigned int value) {

    if (pin < GPIO_PIN_FIRST || pin > GPIO_PIN_LAST) {
        return; 
    }

    // if we're setting it high
    if (value == 1) {
	    gpio->set[pin / 32] = 1 << (pin % 32);
    
    } else if (value == 0) { // if we're setting it low
        gpio->clr[pin / 32] = 1 << (pin % 32);

    } else {
        return;
    }
}

/* This reads the LEV register and returns the bit
 * at the given pin
 */
unsigned int gpio_read(unsigned int pin) {
    if (pin < GPIO_PIN_FIRST || pin > GPIO_PIN_LAST) {
        return GPIO_INVALID_REQUEST; 
    }

   // test that bit
   unsigned int bit_to_read = gpio->lev[pin / 32];
   return 1 & (bit_to_read >> (pin % 32));
}

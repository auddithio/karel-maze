/* 
 * FILE: keyboard.c
 * -----------------------------------------------------
 * Author: Auddithio Nag
 *
 * This code implements the keyboard driver and interface
 * for an external PS2 keyboard. It can read the scancodes
 * sent by the keyboard while typing, package those 
 * scancodes into events (press/release), implements
 * special characters by using modifiers (eg. SHIFT, ALT,
 * CAPS LOCK). Overall, it converts our scancodes into
 * a fleshed out text typing system.
 *
 * Happy typing!
 */

#include "keyboard.h"
#include "ps2.h"

static ps2_device_t *dev;

// to track state
static int state = 0; 
static int caps_on = 0;

void keyboard_init(unsigned int clock_gpio, unsigned int data_gpio)
{
    dev = ps2_new(clock_gpio, data_gpio);
}

unsigned char keyboard_read_scancode(void)
{
    return ps2_read(dev);
}

key_action_t keyboard_read_sequence(void)
{
    key_action_t action;
    unsigned char scancode = keyboard_read_scancode();

    // if it's an extended key, read the next one
    if (scancode == 0xe0) {
        scancode = keyboard_read_scancode();
    }

    // determine release or press
    if (scancode == 0xf0) {
        action.what = KEY_RELEASE;
        scancode = keyboard_read_scancode();
    } else {
        action.what = KEY_PRESS;
    }

    // determine the actual keycode
    action.keycode = scancode;
    
    return action;
}

/* 
 * This function changes the current state in response to the 
 * press/release of modifier keys (SHIFT, ALT, CTRL, CAPS LOCK).
 * If non-modifier keys are input, there's no change.
 *
 * @params  key event
 * @returns none
 */
void change_state(key_event_t event) {

    unsigned char key = event.key.ch;
    int move = event.action.what;

    // "non-sticky" modifier: press/release toggles on/off
    unsigned int bit_flag = 0;

    if (key == PS2_KEY_SHIFT) {
        bit_flag = KEYBOARD_MOD_SHIFT;
        
    } else if (key == PS2_KEY_ALT) {
        bit_flag = KEYBOARD_MOD_ALT;
        
    } else if (key == PS2_KEY_CTRL) {
        bit_flag = KEYBOARD_MOD_CTRL;
    }

    // add/remove modifier for press/release
    // add = bitwise and, remove = bitwise or with ~bit_flag
    state = (move == KEY_PRESS) ? state | bit_flag : state & ~bit_flag;

    if (bit_flag != 0) return; 

    // "sticky" keys: press once to turn on, another time for off
    if (key == PS2_KEY_CAPS_LOCK) {
        bit_flag = KEYBOARD_MOD_CAPS_LOCK;
    }

    // switch modifier value only on press
    if (move == KEY_PRESS && caps_on == 0) {

        // switching is equivalent to state XOR bit_flag      
        state = (state | bit_flag) & (~state | ~bit_flag);
        caps_on = 1;

    } else if (move == KEY_RELEASE) {
        caps_on = 0; // KEY_RELEASE resets CAPS, so that it doesn't oscillate
    }
}

key_event_t keyboard_read_event(void)
{
    key_event_t event;

    // set new state 
    while (1) {

        event.action = keyboard_read_sequence();
        event.key = ps2_keys[event.action.keycode];

        // if it's a non-modifier key
        if (event.key.ch < PS2_KEY_SHIFT 
                    || event.key.ch > PS2_KEY_CAPS_LOCK) { 
            break;
        }

        change_state(event);

    }

    // check modifiers
    event.modifiers = state;
    return event;
}

unsigned char keyboard_read_next(void)
{
    key_event_t event = keyboard_read_event();

    // if it's a release, look at next key
    while (event.action.what == KEY_RELEASE) {
        event = keyboard_read_event();
    }
    
    unsigned char character = event.key.ch;

    // if SHIFT was on
    if ((event.modifiers & KEYBOARD_MOD_SHIFT)

            // or if CAPS was toggled on while on a letter
            || (event.modifiers & KEYBOARD_MOD_CAPS_LOCK
                && character >= 'a' && character <= 'z')) {

        character = event.key.other_ch;
    }

    return character;
}

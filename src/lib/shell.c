/* 
 * FILE: shell.c
 * --------------------------------------------------------
 * Author: Auddithio Nag
 *
 * This file implements a basic shell using the PS2 keyboard.
 * The user can type into the shell terminal, and use a selected 
 * number of commands, listed below in "commands". 
 *
 * This file also implements the extension, where the user can move 
 * the cursor left or right using the left/right arrows respectively,
 * and can use the "history" command to see past commands. It also 
 * implements the profiler extension.
 *
 * Happy typing!
 */

#include "shell.h"
#include "shell_commands.h"
#include "uart.h"
#include "strings.h"
#include "malloc.h"
#include "pi.h"
#include "ps2_keys.h"
#include "armtimer.h"
#include "interrupts.h"
#include "backtrace.h"

// for profiler extension
extern unsigned int __text_end__;
static unsigned int text_end = (unsigned int) &__text_end__;
static unsigned int text_start = 0x8000;

const int NUM_INSTR = 20; // the number of instructions to print with the most hospot counts
static unsigned int *counts = NULL; // pointer to array of hotspot counts
const unsigned int COUNT_PERIOD = 1000; // time between interrupt counts

#define LINE_LEN 80
const unsigned int MAX_TOKENS = LINE_LEN / 2; // max when each command/arg
                                              // is one letter and space
                                              
static input_fn_t shell_read;
static formatted_fn_t shell_printf;

// track all commands typed in shell
#define MAX_HISTORY 20
char *history[MAX_HISTORY];
unsigned int num_history= 0; // # of calls saved in history

int cmd_history(int argc, const char *argv[]);
int cmd_profile(int argc, const char *argv[]);

// NOTE TO STUDENTS: It will greatly help our grading if you use the following
// format strings in the following contexts. We provide the format strings; you
// will need to determine what the right arguments (if any) should be.
//
// When the user attempts to execute a command that doesn't exist:
// "error: no such command `%s`. Use `help` for list of available commands.\n"
//
// When the user attempts to run `help <cmd>` for a command that doesn't exist:
// "error: no such command `%s`.\n"
//
// When the user attempts to `peek` or `poke` an address that is not 4-byte aligned:
// "error: %s address must be 4-byte aligned\n"
//
// When running `peek` on an address, the resulting printout should be formatted thus:
// "%p:   %08x\n"
static const command_t commands[] = {
    {"help",   "<cmd> prints a list of commands or description of cmd", cmd_help},
    {"echo",   "<...> echos the user input to the screen", cmd_echo},
    {"reboot", "reboots the Raspberry pi back to the bootloader", cmd_reboot},
    {"peek", "[address] print contents of memory at address", cmd_peek},
    {"poke", "[address] [value] stores value at address", cmd_poke},
    {"history", "prints out the commands typed till now", cmd_history},
    {"profile", "[on | off] shows the hotspots in the code", cmd_profile},
};

const unsigned int NUM_COMMANDS = sizeof(commands) / sizeof(command_t);

/* 
 * Checks whether a command is included in the command table 
 * or not, and returns the index of the command in the table
 * if found.
 *
 * @params  command name
 * @returns -1 (if command not found), index of command (if found)
 */
int is_command_valid(const char *cmd_name) {

    for (int i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(commands[i].name, cmd_name) == 0) {
            return i;
        }
    }
 
    return -1;
}

int cmd_echo(int argc, const char *argv[])
{
    for (int i = 1; i < argc; ++i)
        shell_printf("%s ", argv[i]);
    shell_printf("\n");
    return 0;
}

int cmd_help(int argc, const char *argv[])
{
    // if only "help" is typed, print out all command names & descriptions
    if (argc == 1) {
        for (int i = 0; i < NUM_COMMANDS; i++) {
            shell_printf("%s: %s\n", commands[i].name, commands[i].description);
        }

    } else {
        // look up first argument
        int cmd_index = is_command_valid(argv[1]);
        
        // if first arg is not a command
        if (cmd_index == -1) {
            shell_printf("error: no such command '%s'.\n", argv[1]);
            return 1;
        }
        
        // else describe that command
        shell_printf("%s: %s\n", commands[cmd_index].name, commands[cmd_index].description);
    }
        
    return 0;
}

int cmd_reboot(int argc, const char *argv[])
{
    uart_putchar(EOT);
    pi_reboot();
}

int cmd_peek(int argc, const char *argv[]) {

    // error if only 'peek' typed
    if (argc == 1) {
        shell_printf("error: peek requires 1 argument [address]\n");
        return 1;
    }

    const char *endptr;
    unsigned int *addr = (unsigned int *)strtonum(argv[1], &endptr);

    // address argument is invalid
    if (*endptr != '\0') {
        shell_printf("error: peek cannot convert %s\n", argv[1]);
        return 1;
    }
    
    // address isn't 4-byte aligned
    if ((unsigned int) addr % 4 != 0) {
        shell_printf("error: %s address must be 4-byte aligned\n", argv[0]);
        return 1;
    }

    shell_printf("0x%08x: %08x\n", (unsigned int) addr, *addr);
    return 0;
}

int cmd_poke(int argc, const char *argv[]) 
{
    // error if only 'poke' typed
    if (argc < 3) {
        shell_printf("error: poke requires 2 arguments [address] [value]\n");
        return 1;
    }   

    const char *endptr1, *endptr2;
    unsigned int *addr = (unsigned int *)strtonum(argv[1], &endptr1);
    unsigned int val = strtonum(argv[2], &endptr2);

    // address argument is invalid
    if (*endptr1 != '\0') {
        shell_printf("error: poke cannot convert %s\n", argv[1]);
        return 1;
    }

    // value is invalid
    if (*endptr2 != '\0') {
        shell_printf("error: poke cannot convert %s\n", argv[2]);
        return 1;
    }
    
    // address isn't 4-byte aligned
    if ((unsigned int) addr % 4 != 0) {
        shell_printf("error: %s address must be 4-byte aligned\n", argv[0]);
        return 1;
    }

    *addr = val; // store value
    return 0;
}

/* 
 * Prints out all the commands typed out till now in our shell,
 * oldest one first. Ignores all arguments
 *
 * @returns 0
 * @precon  history array must not be overflown
 */
int cmd_history(int argc, const char *argv[]) {
    for (int i = 1; i < num_history; i++) {
        shell_printf("%d. %s\n", i, history[i]);
    }

    return 0;
}

/* 
 * Handler function to interrupt program and incement
 * the number of counts
 */
void get_counts(unsigned int pc, void *aux_data) {

    if (armtimer_check_and_clear_interrupt()) {
        counts[pc - text_start]++;
    }
}

/*
 * This function prints the hotspots in a program 
 * by reading from the counts array
 */
void print_hotspots() {

    shell_printf("Counts  |  Function  [pc]\n");
    shell_printf("-------------------------------\n");
    
    // print top instructions
    for (int n = 0; n < NUM_INSTR; n++) {

        unsigned int max_counts = 0;
        unsigned int max_addr = 0;

        for (unsigned int i = 0; i < (text_end - text_start) / 4; i++) {
            if (counts[i] > max_counts) {
                max_counts = counts[i];
                max_addr = i;
            }
        }

        // walk back to find name
        unsigned int addr = max_addr * 4 + text_start;
        char *name = (char *)name_of(addr);
        unsigned int offset = 0;
        while (strcmp(name, "???") == 0) {
            addr -= 4;
            offset += 4;
            name = (char *)name_of(addr);            
        }

        shell_printf("%d  |  %s+%d  [0x%x]\n", max_counts, name,
                    offset, max_addr * 4 + text_start);

        // set highest to 0
        counts[max_addr] = 0;
    }
       
}

/*
 * Implements the profiler extension. Enables timer that interrupts
 * program to get count of hotspots
 */
int cmd_profile(int argc, const char *argv[]) {
    if (argc != 2) {
        shell_printf("error: profile needs 1 argument (on/off)\n");
        return 1;
   
    } else {

        if (strcmp(argv[1], "on") == 0) {

            counts = (unsigned int *)malloc(text_end - text_start);

            // zero out counts
            for (int i = 0; i < (text_end - text_start) / 4; i++) {
                counts[i] = 0;
            }

            // start timer
            armtimer_enable();

        } else if (strcmp(argv[1], "off") == 0) {

            // stop counts
            armtimer_disable();
            free(counts);
            
            // print out results
            print_hotspots();

        } else {
            shell_printf("error: argument for profiel should be 'on' or 'off'\n");
        }

        return 0;
    }
}


void shell_init(input_fn_t read_fn, formatted_fn_t print_fn)
{
    shell_read = read_fn;
    shell_printf = print_fn;

    // configure armtimer for profile
    armtimer_init(COUNT_PERIOD);
    armtimer_enable_interrupts();
    interrupts_register_handler(INTERRUPTS_BASIC_ARM_TIMER_IRQ, get_counts, NULL);
    interrupts_enable_source(INTERRUPTS_BASIC_ARM_TIMER_IRQ);
}

void shell_bell(void)
{
    uart_putchar('\a');
}

/* 
 * This function implements the code to call when 
 * backspace is typed into the shell console
 *
 * @params  current index of cursor in shell buffer
 * @returns final index of cursor in shell buffer
 */
int backspace(int cur_index, int final_index, char buf[], size_t bufsize)
{
    // no backspace allowed at start!           
    if (cur_index == 0) {         
        shell_bell();
            
    } else if (cur_index == final_index) {                
        shell_printf("\b \b");                   
        buf[--cur_index] = '\0';

    } else {

        // delete that character, then concatenate
        buf[cur_index - 1] = '\0';
        strlcat(buf, buf + cur_index, bufsize);

        // print change to shell
        shell_printf("\b");
        shell_printf("%s", buf + cur_index - 1);
        shell_printf(" \b");
        
        // move cursor back to where it was
        for (int i = 0; i < final_index - cur_index; i++) {
            shell_printf("\b");
        }

        cur_index--;
    }
    return cur_index;
}

/*
 * This function shifts a block of characters (up to 
 * a null terminator) in a buf downstream/right by
 * 1 letter.
 *
 * @params  buffer, index of first character to move
 * @returns none
 * @precon  string in buffer must be null terminated,
 *          buffer must have space
 */
void shift_characters_right(char buf[], int cur_index) {
    char temp = buf[cur_index];
    int new_index = cur_index + 1;

    while (1) {
        char new_temp = buf[new_index];
        buf[new_index++] = temp;
        temp = new_temp;

        if (buf[new_index - 1] == '\0') break;
    }
}
        

void shell_readline(char buf[], size_t bufsize)
{
    unsigned char key = 0;
    int cur_index = 0; // index our cursor is at
    int final_index = 0; // index at end of shell string
    
    // store in history
    history[num_history++] = malloc(LINE_LEN);

    while (1) {
        key = shell_read();

        if (key == '\n') {
            shell_printf("\n");
            return;

        // if we're at max size: raise alert and don't write
        } else if (final_index == bufsize) {
            shell_bell();

        // a backspace is typed
        } else if (key == '\b') {
            int new_cur_index = backspace(cur_index, final_index, buf, bufsize);
            if (cur_index > new_cur_index) {
                cur_index = new_cur_index;
                final_index--;
            }

        // use left arrow to move cursor left
        } else if (key == PS2_KEY_ARROW_LEFT) {
            if (cur_index == 0) {
                shell_bell();
            } else {
                shell_printf("\b");
                cur_index--;
            }

        // use right arrow to move cursor right
        } else if (key == PS2_KEY_ARROW_RIGHT) {
            if (cur_index == final_index) {
                shell_bell();
            } else {
                // to move cursor right, retype that character
                shell_printf("%c", buf[cur_index++]);
            }
                
        // if it's a key & cursor is at end, append to buf and show on screen
        } else if (cur_index == final_index) {

            buf[cur_index] = key;
            buf[++final_index] = '\0';
            shell_printf("%c", buf[cur_index++]);

            memcpy(history[num_history - 1], buf, final_index + 1); // store

        // cursor is not at end
        } else {
            // reprint characters entered in new cursor position
            shift_characters_right(buf, cur_index);
            buf[cur_index] = key;
            final_index++;

            // reprint 
            shell_printf("%s", buf + cur_index++);
            
            // move cursor back
            for (int i = 0; i < final_index - cur_index; i++) {
                shell_printf("\b");
            }

            memcpy(history[num_history - 1], buf, final_index + 1);

        }
    }
}

/* 
 * Returns whether a character is valid and included in a token
 * or not. To be valid, it shouldn't be a space character ('\n',
 * '\t' or ' ') or a null terminator
 *
 * @params  character
 * @returns 1 (if valid), 0 (if not valid)
 */
int is_valid_char(const char character) 
{
    return character != '\t' && character != '\n' &&
        character != '\0' && character != ' ';
}

int shell_evaluate(const char *line)
{
    // array of string pointers
    char *tokens[MAX_TOKENS]; 
    int num_tokens = 0;

    // while we haven't reached end of line
    while (1) {

        char *start = (char *)line;

        // delineate one token
        while (is_valid_char(*line)) {
            line++;
        }

        // if we found a token, add it to array
        int length = line - start;
        if (length > 0) {
            tokens[num_tokens] = malloc(length + 1);
            memcpy(tokens[num_tokens], start, length);
            tokens[num_tokens++][length] = '\0';
        }

        if (*line == '\0') break;
        line++;
    }

    // first argument is command
    int cmd_index = is_command_valid(tokens[0]);
    int result = -1;

    if (cmd_index != -1) {
        result = commands[cmd_index].fn(num_tokens, (const char **)tokens);
    } else {
        shell_printf("error: no such command `%s`. Use `help` for list of available commands.\n",
                        tokens[0]);
    }

    // free tokens
    for (int i = 0; i < num_tokens; i++) {
        free(tokens[i]);
    }

    return result;
}

void shell_run(void)
{
    shell_printf("Welcome to the CS107E shell. Remember to type on your PS/2 keyboard!\n");
    while (1)
    {
        char line[LINE_LEN];

        shell_printf("Pi> ");
        shell_readline(line, sizeof(line));
        shell_evaluate(line);
    }
}

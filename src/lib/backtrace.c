/*
 * File: backtrace.c
 * ------------------
 * Author: Auddithio Nag
 *
 * This file implememts the backtrace operation in the 
 * gdb debugger. It uses the stack to help print out
 * the caller function name, resume address and offset,
 * up to a certain number of frames
 */


#include "backtrace.h"
#include "printf.h"

// The most significant byte in a word has 0xff 
// if teh function has a name
const unsigned int MSB_FOR_NAMES = 0xff << 24;

const char *name_of(uintptr_t fn_start_addr)
{
    // find address of word preceding instruction
    unsigned int *prev_word = (unsigned int *) fn_start_addr - 1;

    // if MSB = 0xff it has a name
    if (*prev_word < MSB_FOR_NAMES) {
        return "???";
    }

    // last 3 bytes give us the length of name
    unsigned int length = *prev_word - MSB_FOR_NAMES;
    prev_word -= (length / 4);
    return (char *) prev_word;
}

int backtrace (frame_t f[], int max_frames)
{
    // get current fp
    unsigned int **cur_fp;
    __asm__("mov %0, fp" : "=r" (cur_fp));

    // keep track of number of frames
    int num_frames = 0;

    // while we don't reach top of stack (saved fp isn't 0)
    while (*(cur_fp - 3) != 0) { 

        if (num_frames >= max_frames) {
            break;
        }

        // build current frame
        frame_t cur_frame;

        // saved lr gives resume address
        cur_frame.resume_addr = (unsigned int) *(cur_fp - 1);

        // offset = resume addr - 1st instruction (*saved_pc - 12)
        unsigned int first_instr = **(cur_fp  - 3) - 12;
        cur_frame.resume_offset = cur_frame.resume_addr - first_instr;      

        // update fp
        cur_fp = (unsigned int **) *(cur_fp - 3); 

        cur_frame.name = name_of((unsigned int) *cur_fp - 12);

        // add it to array
        f[num_frames++] = cur_frame;
    }

    return num_frames;
}

void print_frames (frame_t f[], int n)
{
    for (int i = 0; i < n; i++)
        printf("#%d 0x%x at %s+%d\n", i, f[i].resume_addr, f[i].name, f[i].resume_offset);
}

void print_backtrace (void)
{
    int max = 50;
    frame_t arr[max];

    int n = backtrace(arr, max);
    print_frames(arr+1, n-1);   // print frames starting at this function's caller
}

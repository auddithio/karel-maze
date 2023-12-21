/*
 * File: malloc.c
 * --------------
 * Author: Auddithio Nag
 *
 * This code implements a dynamic heap allocator. An allocation 
 * request is serviced by using sbrk to extend the heap segment.
 * It can also recycle memory that has been deallocated. It does
 * so by freeing and coalescing unused blocks, and refilling them 
 * with new data when possible. Each payload has a header at its 
 * start to help do this.
 *
 * It also implements the extension; a mini-Valgrind that provides
 * red zone protection by using 2 flanking red zones around the 
 * payload. It also prints a report of the total number of allocations 
 * and frees and also any memory leaks at the end. The memory_report()
 * function is called in _cstart.c
 */

#include "malloc.h"
#include "printf.h"
#include <stddef.h> // for NULL
#include "strings.h"
#include "backtrace.h"
#include "assert.h"

extern int __bss_end__;

const unsigned int MIN_BLOCK_SIZE = 8;
const unsigned int RED_ZONE_LEN = 0;
const unsigned char RED_ZONE_CHAR = '~'; // 0x7e
const unsigned int NUM_FRAMES = 0; // # of backtrace frames we want

enum {FREE, USED};

// The header of each payload we allocate
typedef struct {
    size_t payload_size; // memory allocated for payload
    int status; // whether it's free (0) or used (1)
    size_t data_size; // the actual size of payload 
    frame_t frames[3]; // 3 backtrace frames
} header;

const unsigned int HEADER_SIZE = sizeof(header);

// global variables to track aggregate heap statistics
header heap_usage[5000]; // can track at most 50 alloc pairs
int num_allocs = 0;
int num_frees = 0;
unsigned int total_bytes = 0;

void report_damaged_redzone (void *ptr);

/*
 * The pool of memory available for the heap starts at the upper end of the
 * data section and extend up from there to the lower end of the stack.
 * It uses symbol __bss_end__ from memmap to locate data end
 * and calculates end of stack reservation assuming a 16MB stack.
 *
 * Global variables for the bump allocator:
 *
 * `heap_start`  location where heap segment starts
 * `heap_end`    location at end of in-use portion of heap segment
 */

// Initial heap segment starts at bss_end and is empty
static void *heap_start = &__bss_end__;
static void *heap_end = &__bss_end__;

// Call sbrk as needed to extend size of heap segment
// Use sbrk implementation as given
void *sbrk(int nbytes)
{
    void *sp;
    __asm__("mov %0, sp" : "=r"(sp));   // get sp register (current stack top)
    char *stack_reserve = (char *)sp - 0x1000000; // allow for 16MB growth in stack

    void *prev_end = heap_end;
    if ((char *)prev_end + nbytes > stack_reserve) {
        return NULL;
    } else {
        heap_end = (char *)prev_end + nbytes;
        return prev_end;
    }
}


// Simple macro to round up x to multiple of n.
// The efficient but tricky bitwise approach it uses
// works only if n is a power of two -- why?
#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))

/* 
 * This function finds the first available free space
 * in the heap that has a payload size of 'nbytes'. The 
 * free space can be anywhere from the heap_start to 
 * all the way to the heap_end.
 *
 * @param   payload size
 * @returns pointer to header of free memory
 * @precon  nbytes >= 0
 */
header *find_space(size_t nbytes) {
    header *cur = heap_start;

    // find available space
    while (cur != heap_end) {

        // if space available in freed memory
        if (cur->status == FREE // block is free
                && cur->payload_size >= nbytes) { // has space
            break;

        } else {
            // jump to next header
            char *new_head = (char *)cur + cur->payload_size + HEADER_SIZE 
                    + 2 * RED_ZONE_LEN;
            cur = (header *)new_head;
        }
    }

    return cur;
}

/* 
 * This function intialises the red zones of each new payload with
 * the character of choice (defined by RED_ZONE_CHAR)
 *
 * @params  header of payload
 * @returns none
 */
void initialise_redzones(header *head) {
    char *redzone = (char *)head + HEADER_SIZE;
    
    // leading redzone
    memset(redzone, RED_ZONE_CHAR, RED_ZONE_LEN);

    // trailing redzone
    memset(redzone + RED_ZONE_LEN + head->data_size, 
            RED_ZONE_CHAR, RED_ZONE_LEN);
}

/* 
 * When allocating new memory to a freed payload, this function
 * creates a new header and payload if there's still space left.
 *
 * @params  pointer to current header, current payload size
 * @returns none
 * @precon  payload must have at least enough space for the smallest
 *          payload possible (plus header and red zones)
 */
void split_block(header *cur_head, size_t nbytes) {
    unsigned int prev_block_size = cur_head->payload_size;

    // make new header
    char *next = (char *)cur_head + nbytes + HEADER_SIZE 
                    + 2 * RED_ZONE_LEN;
    header *new_head = (header *)next;

    new_head->payload_size = prev_block_size - nbytes - HEADER_SIZE 
                                - 2 * RED_ZONE_LEN;
    new_head->status = FREE; 
}

void *malloc (size_t nbytes)
{

    if (nbytes <= 0) { // error checking
        return NULL;
    }
    
    unsigned int orig_size = nbytes;
    nbytes = roundup(nbytes, MIN_BLOCK_SIZE);

    // allocate space for block and header
    header *head = find_space(nbytes);    

    // extend heap if we're at the heap end
    if ((unsigned int *)head == heap_end) { 
        head = (header *)sbrk(nbytes + HEADER_SIZE + 2 * RED_ZONE_LEN);
        if (head == NULL) return head; // return NULL if heap is full
        head->payload_size = nbytes;

    // if it's a free block    
    } else {        
        // if there's space to split the block
        if (head->payload_size >= nbytes + HEADER_SIZE + 
                MIN_BLOCK_SIZE + 2 * RED_ZONE_LEN) {
            split_block(head, nbytes); // create smaller block too
            head->payload_size = nbytes;
        }
    }

    if (!head) {
        return NULL;
    }

    // initialise new header
    head->status = USED;        
    head->data_size = orig_size;

    // mini-Valgrind protections
    initialise_redzones(head);
    backtrace(head->frames, NUM_FRAMES);
    total_bytes += orig_size;

    heap_usage[num_allocs++] = *head;

    char *payload = (char *)head + HEADER_SIZE + RED_ZONE_LEN;
    return payload;
}

/*
 * This function checks the flanking red zones of a payload
 * to make sure they haven't been modified/corrupted.
 * Prints a message and a backtrace of its allocation if 
 * they are modified.
 *
 * @params  payload header
 * @returns none
 */
void check_redzones(header *head) {

    char *leading_redzone = (char *)head + HEADER_SIZE;
    char *trailing_redzone = (char *)head + HEADER_SIZE + RED_ZONE_LEN 
                                + head->data_size;
    
    // check if they have the right character
    for (int i = 0; i < RED_ZONE_LEN; i++) {
        if (leading_redzone[i] != RED_ZONE_CHAR ||
                trailing_redzone[i] != RED_ZONE_CHAR) {
            report_damaged_redzone(leading_redzone + RED_ZONE_LEN);
            break;
        }
    }    
}

void free (void *ptr)
{
    // ignore null pointer
    if (!ptr) {
        return;
    }
    
    // make pointer to header
    char *head_char = (char *)ptr - HEADER_SIZE - RED_ZONE_LEN;
    header *head = (header *)head_char;
    
    // mini-Valgrind protections
    check_redzones(head); // check if memory hasn't been overstepped 
    num_frees++;
    
    for (int i = 0; i < num_allocs; i++) {

        // find right frame by finding the original allocation
        if (*(heap_usage[i].frames[1].name) == *(head->frames[1].name)
                && heap_usage[i].frames[1].resume_offset == head->frames[1].resume_offset) { 
            heap_usage[i].status = FREE; // mark it off
        }
    }
   
    head->status = FREE; 

    // find header of next block
    head_char += head->payload_size + HEADER_SIZE + 2 * RED_ZONE_LEN;
    header *next = (header *)head_char;

    // while we're not at end, and it's free, coalesce
    while (next != heap_end && next->status == 0) {

        // update size in header
        int block_size = next->payload_size + HEADER_SIZE + 2 * RED_ZONE_LEN;
        head->payload_size += block_size;
        head_char += block_size;
        next = (header *)head_char;
    }
}


void heap_dump (const char *label)
{
    printf("\n---------- HEAP DUMP (%s) ----------\n", label);
    printf("Heap segment at %p - %p\n", heap_start, heap_end);
    
    header *cur = heap_start;
    while (cur != heap_end) {

        char *data = (char *)cur + HEADER_SIZE + RED_ZONE_LEN;

        // print content
        printf("Address: %p, Block size: %d, Status: %d\n", data, 
                cur->payload_size, cur->status);

        // jump to next header
        int offset = cur->payload_size + HEADER_SIZE + 2 * RED_ZONE_LEN;
        char *new_header = (char *)cur + offset;
        cur = (header *)new_header;
    }

    printf("----------  END DUMP (%s) ----------\n", label);
}

void memory_report ()
{
    printf("\n=============================================\n");
    printf(  "         Mini-Valgrind Memory Report         \n");
    printf(  "=============================================\n");

    printf("malloc/free: %d allocs, %d frees, %d bytes allocated.\n", 
            num_allocs, num_frees, total_bytes);

    // look for memory leaks
    if (num_allocs != num_frees) {

        // find the leaked memories
        for (int i = 0; i < num_allocs; i++) {
            if (heap_usage[i].status == 1) {

                // print out error report
                printf("%d bytes lost, allocated by:\n", 
                        heap_usage[i].data_size);
                print_frames(heap_usage[i].frames, NUM_FRAMES);
            }
        }
    }
    
}

void report_damaged_redzone (void *ptr)
{
    printf("\n=============================================\n");
    printf(  " **********  Mini-Valgrind Alert  ********** \n");
    printf(  "=============================================\n");
    printf("Attempt to free address %p that has damaged red zone(s):", ptr);
    
    // find the header
    unsigned int *head_int = (unsigned int *)ptr - (HEADER_SIZE + 
                                RED_ZONE_LEN) / sizeof(unsigned int);
    header *head = (header *)head_int;
    
    // print out values in redzones
    printf(" [%x], [%x]\n", *(head_int + HEADER_SIZE/sizeof(unsigned int)), 
            *(head_int + (HEADER_SIZE + RED_ZONE_LEN + head->data_size) 
                            / sizeof(unsigned int)));

    printf("Block of size %d bytes, allocated by:\n", head->data_size);
    print_frames(head->frames, NUM_FRAMES);

}

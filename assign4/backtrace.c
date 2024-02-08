/* File: backtrace.c
 * -----------------
 * ***** TODO: add your file header comment here *****
 */
#include "backtrace.h"
#include "printf.h"

// helper function implemented in file backtrace_asm.s
extern unsigned long backtrace_get_fp(void);

int backtrace_gather_frames (frame_t f[], int max_frames) {
    /***** TODO: Your code goes here *****/
    return 0;
}

void backtrace_print_frames (frame_t f[], int n) {
    /***** TODO: Your code goes here *****/
}

void backtrace_print (void) {
    int max = 50;
    frame_t arr[max];

    int n = backtrace_gather_frames(arr, max);
    backtrace_print_frames(arr+1, n-1);   // print frames starting at this function's caller
}

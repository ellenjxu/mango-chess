/* File: backtrace.c
 * -----------------
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 * Backtrace functions for Mango Pi.
 */
#include "backtrace.h"
#include "printf.h"

// helper function implemented in file backtrace_asm.s
extern unsigned long backtrace_get_fp(void);

/*
 * Always do arithmetic on uintptr_t and then cast to unsigned long *
 * so we can add raw bytes and don't need to worry about dividing by
 * sizeof(long).
*/
int backtrace_gather_frames(frame_t f[], int max_frames) {
    uintptr_t fp = backtrace_get_fp();

    // move up one call (look at previous fp)
    fp = *(uintptr_t *)(fp - 16);

    int count = 0;

    while (count < max_frames) {
        // resume address is written at fp - 8
        f[count].resume_addr = *(uintptr_t *)(fp - 8);

        // caller fp is written at fp - 16
        fp = *(uintptr_t *)(fp - 16);

        count++;

        // top of stackframe
        if ((uintptr_t *)fp == NULL)
            break;
    }

    return count;
}

void backtrace_print_frames(frame_t f[], int n) {
    for (int i = 0; i < n; i++) {
        // Mask out the leading 4 in the addr:  0x40000000
        unsigned long addr = f[i].resume_addr & 0x0fffffff;
        printf("#%d %p at <.text+%ld>\n", i, (void *)f[i].resume_addr, addr);
    }
}

void backtrace_print(void) {
    int max = 50;
    frame_t arr[max];

    int n = backtrace_gather_frames(arr, max);
    backtrace_print_frames(arr+1, n-1);   // print frames starting at this function's caller
}

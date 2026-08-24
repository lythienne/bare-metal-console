/* File: backtrace.c
 * -----------------
 * Adds support to gather stack frames using the frame pointer and print a backtrace. 
 * Also adds guards to the stack with a fixed canary to protect against stack 
 * overflow.
 *
 * Author: Harrison Chen
 * Version: 2/9/26
 */
#include "backtrace.h"
#include "mango.h"
#include "printf.h"
#include "symtab.h"
#include <stdint.h>

// helper function implemented in file backtrace_asm.s
extern unsigned long backtrace_get_fp(void);

// gathers return addresses in a list by jumping up stack with frame ptrs
int backtrace_gather_frames(frame_t f[], int max_frames) {
    uintptr_t* fp = (unsigned long *) backtrace_get_fp();
    for(int i = 0; i < max_frames; i++) {
        if (fp == NULL) {
            return i;
        }
        f[i].resume_addr = *(fp - 1);
        fp = (unsigned long *) *(fp - 2);
    }
    return max_frames;
}

void backtrace_print_frames(frame_t f[], int n) {
    char labelbuf[128];

    for (int i = 0; i < n; i++) {
        symtab_label_for_addr(labelbuf, sizeof(labelbuf), f[i].resume_addr);
        printf("#%d 0x%08lx at %s\n", i, f[i].resume_addr, labelbuf);
    }
}

void backtrace_print(void) {
    int max = 50;
    frame_t arr[max];

    int n = backtrace_gather_frames(arr, max);
    backtrace_print_frames(arr+1, n-1);   // print frames starting at this function's caller
}


long __stack_chk_guard = 0xBBBB67edBBBB67ed;

// if canary mismatch detected, print out the 
void __stack_chk_fail(void)  {
    char buf[128];
    const int RA_OFFSET = -8;       // ra stored on stack 8 words away from usual place
    uintptr_t ra = ((unsigned long *) backtrace_get_fp())[-1 + RA_OFFSET];
    symtab_label_for_addr(buf, sizeof(buf), ra);
    const char *fn_name = buf;
    printf("\n *** Stack smashing detected at end of function %s() *** \n", fn_name);
    mango_abort();
}

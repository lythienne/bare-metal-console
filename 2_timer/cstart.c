/* File: cstart.c
 * --------------
 * This file contains the C language portion of start sequence.
 * This code provided to you pre-written for Assignment 2. You
 * should not edit it.
 * In an upcoming lecture, we will discuss what this code does
 * and why it is necessary. Stay tuned!
 */

#include "memmap.h"

// The C function _cstart is called from the assembly in start.s
// _cstart zeroes out the BSS section and then calls main.
// Before starting main(), turns on the blue ACT LED and
// turns off after as a sign of successful completion.
void _cstart(void)
{
    // zero out all bytes in BSS section, markers from linker script for boundaries
    for (uint8_t *bss = BSS_START; bss < BSS_END; bss++) {
        *bss = 0;
    }

    // Turn on the blue act led (GPIO PD18) before starting main
    volatile unsigned int *PD_CFG2 = (unsigned int *)0x02000098;
    volatile unsigned int *PD_DATA = (unsigned int *)0x020000a0;
    unsigned int bit_18 = 1 << 18;
    *PD_CFG2 = (*PD_CFG2 & ~0xf00) | 0x100;  // configure PD18 for output
    *PD_DATA |= bit_18;    // turn on PD18

    main();

    *PD_DATA &= ~bit_18;    // turn off led after main finishes normally
}
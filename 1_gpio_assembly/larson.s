/* File: larson.s
 * --------------
 * Code to create a larson scanner with 8 LEDs.
 * 
 * Author: Harrison Chen
 * Version: 1/14/26
 */

    # config
    lui     a0,0x2000       # a0 holds base addr PB group = 0x2000000
    li      t0,0x11111111   # config PB0-PB7 as output
    sw      t0,0x30(a0)         

    # starting values
    li      a1,1            # a1 holds which light to turn on
    li      t1,1            # t1 holds direction (forward = 1, back = -1)
    li      t2,0b00000001   # t2 holds comparison value on where to reverse (back)
    li      t3,0b10000000   # t3 holds comparison value on where to reverse (forward)

loop:
    sw      a1,0x40(a0)     # a1 chooses which light to turn on
    blt     t1,zero,back    # if direction < 0 (backwards) branch to backward
forward:
    slli    a1,a1,1         # shift light to turn on left (forward)
    bne     a1,t3,after     # if not at 8th light yet jump to after
    li      t1,-1           # else switch direction to back
    j       after
back:
    srli    a1,a1,1         # shift light to turn on right (backward)
    bne     a1,t2,after     # if not at first light yet jump to after
    li      t1,1            # else switch direction to forward
after:
    lui     a2,1000         # a2 = init countdown value
delay:
    addi    a2,a2,-1        # decrement a2
    bne     a2,zero,delay   # keep counting down until a2 is zero

    j       loop            # back to top of outer loop

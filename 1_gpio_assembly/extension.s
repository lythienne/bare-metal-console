/* File: extension.s
 * --------------
 * Code to create a (cooler) larson scanner with 8 LEDs. Dims the surrounding LEDs with
 * manual PWM. Button controlled hang state to better see the brightness gradation.
 * 
 * Author: Harrison Chen
 * Version: 1/15/26
 */

    # config
    lui     a0,0x2000       # a0 holds base addr PB group = 0x2000000
    li      t0,0x11111111   # config PB0-PB7 as output
    sw      t0,0x30(a0)         
    sw      zero,0x60(a0)   # config PC0 as input

    # starting values
    li      s1,1            # s1 holds constant 1
    li      a1,1            # a1 holds the center bit to turn on in the pattern (starts at the lowest)
    li      t1,1            # t1 holds direction (forward = 1, back = -1; start going forward)
    li      t2,0b00000001   # t2 holds comparison value on where to reverse (back)
    li      t3,0b10000000   # t3 holds comparison value on where to reverse (forward)

loop:
    # set up a1 = center bit (...1...), a2 = 3 bits around center (...111...), a3 = 5 bits around center (...11111...)
    slli    t4,a1,1         # shift the on bit in a1 left, store in t4 (temp)
    or      a2,t4,a1        # store both the 1s in t4 and a1 into a2
    slli    t4,t4,1         # shift the on bit in t4 left again
    or      a3,a2,t4        # store all three bits into a3

    srli    t4,a1,1         # shift the on bit in a2 right, store in t4 (temp)
    or      a2,t4,a2        # store the 1 in t4 into a2, a2 contains the center on bit and 1 bit to either side
    or      a3,t4,a3        # store the 1 in t4 into a3
    srli    t4,t4,1         # shift the on bit in t4 right again
    or      a3,a3,t4        # store the 1s in t4 and a2 into a3, a3 contains the center on bit and 2 bits to either side

    lui     t5,0x8          # t5 = init countdown for PWM light brightness control
light_loop:
    sw      a1,0x40(a0)     # turn on only center bit (a1)

    li      t4,0x60         # t4 = init countdown (one light on)
dim1:
    addi    t4,t4,-1        # decrement t4
    bne     t4,zero,dim1    # keep counting down until t4 is zero
    
    sw      a2,0x40(a0)     # turn on 3 bits around center (a2)

    li      t4,0x60         # t4 = init countdown (3 lights on)
dim2:
    addi    t4,t4,-1        # decrement t4
    bne     t4,zero,dim2    # keep counting down until t4 is zero

    sw      a3,0x40(a0)     # turn on 5 bits around center (a3)

    li      t4,0x40         # t4 = init countdown (3 lights on)
dim3:
    addi    t4,t4,-1        # decrement t4
    bne     t4,zero,dim3    # keep counting down until t4 is zero

    lw      t4,0x70(a0)         # load PC0 into t4
    and     t4,t4,s1            # and t4 with 0x1 to clear other data bits
    beq     t4,zero,light_loop  # if button pressed don't decrement t5

    addi    t5,t5,-1            # decrement t5
    bne     t5,zero,light_loop  # keep looping until t5 is zero

    # move center bit
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
    j       loop            # back to top of outer loop

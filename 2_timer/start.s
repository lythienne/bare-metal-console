# File: start.s
# -------------
# This file defines the start sequence. These asm instuctions
# are placed first in binary image and will be the first
# to be executed in a newly loaded program.
#
# This code provided to you pre-written for Assignment 2. You
# should not edit it.
# In an upcoming lecture, we will discuss what this code does
# and why it is necessary. Stay tuned!


 .attribute arch, "rv64im_zicsr"

# Identify this section as the one to go first in binary image
.section ".text.start"

.globl _start
_start:
    csrc    mstatus, 1<<3   # global disable interrupts, mstatus.mie = 0
    la      t0,_trap_handler
    csrw    mtvec,t0        # install trap handler
    lui     sp,0x50000      # init stack at 0x50000000 (grows down)
    jal     _cstart

    hang: j hang            # backstop at end of instructions

.align 8
_trap_handler:
    j _trap_handler        # if exception raised, hang

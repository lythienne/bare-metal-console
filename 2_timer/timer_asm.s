# File: timer_asm.s
# ------------------
# Defines a function to get the machine time from the MangoPi
#
# Author: Harrison Chen
# Version: 1/22/26

.attribute arch, "rv64imac_zicsr"

.globl timer_get_ticks
timer_get_ticks:
    csrr a0, time   # a0 is the return register
    ret

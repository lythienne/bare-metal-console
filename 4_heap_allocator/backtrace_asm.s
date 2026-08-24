# File: backtrace_asm.s
# ---------------------
# Code for a simple assembly function to return the value of fp

.globl backtrace_get_fp
backtrace_get_fp:
    mv a0,fp
    ret

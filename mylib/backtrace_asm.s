/* File: backtrace_asm.s
 * ---------------------
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 */

.globl backtrace_get_fp
backtrace_get_fp:
    mv a0, s0   # retrieve s0/fp register and store it in a0 (a0 = return value)
    ret         # return from function

/* File: timer_asm.s
 * ------------------
 * Javier Garcia Nieto - jgnieto@stanford.edu
 * Assembly code to access CSR register time.
 */

.attribute arch, "rv64imac_zicsr"

.globl timer_get_ticks
timer_get_ticks:
    # Read CSR register time and store in a0 (return value)
    csrr a0, time
    ret

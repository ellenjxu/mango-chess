/* File: cstart.c
 * --------------
 * Provided to you pre-written. In an upcoming lecture, we
 * will discuss what this code does and why it is necessary.
 */

// linker memmap places these symbols at start/end of bss
extern char __bss_start__, __bss_end__;

extern void main(void);

// The C function _cstart is called from the assembly in start.s
// _cstart zeroes out the BSS section and then calls main.
// After return from main(), turns on the onboard act led as
// a sign of successful completion.
void _cstart(void) {
    char *bss = &__bss_start__;
    char *bss_end = &__bss_end__;

    while (bss < bss_end) {
        *bss++ = 0;
    }

    // Turn on the blue act led (GPIO PD18) before starting main
    volatile unsigned int *PD_CFG2 = (unsigned int *)0x02000098;
    volatile unsigned int *PD_DATA = (unsigned int *)0x020000a0;
    *PD_CFG2 = (*PD_CFG2 & ~0xf00) | 0x100;
    *PD_DATA |= (1 << 18);

    main();

    *PD_DATA &= ~(1 << 18); // turn off after main finishes normally
}

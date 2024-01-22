// linker memmap places these symbols at start/end of bss
extern char __bss_start__, __bss_end__;

extern void main(void);
void _cstart(void);

// The C function _cstart is called from the assembly in start.s
// _cstart zeroes out the BSS section and then calls main.
// Before starting main(), turns on the blue ACT LED and
// turns off after as a sign of successful completion.
void _cstart(void)
{
    char *bss = &__bss_start__;
    char *bss_end = &__bss_end__;

    while (bss < bss_end) {
        *bss++ = 0;
    }

    // Turn on the blue act led (GPIO PD18) before starting main
    volatile unsigned int *PD_CFG2 =  (unsigned int *)0x2000098;
    volatile unsigned int *PD_DATA  = (unsigned int *)0x20000a0;
    *PD_CFG2 = (*PD_CFG2 & ~0xf00) | 0x100;  // configure PD18 for output
    *PD_DATA |= 0x40000;    // turn on PD18

    main();

    *PD_DATA &= ~0x40000;    // turn off after main finishes normally
}

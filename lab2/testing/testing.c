// File: testing.c
// ---------------
// Unit testing example

// You call assert on an expression that you expect to be true. If expr
// actually evaluates to false, then assert calls abort, which stops
// your program and flashes the blue ACT LED.
#define assert(expr) if(!(expr)) abort()

void abort(void) {
    // blue ACT LED is controlled by GPIO PD18
     volatile unsigned int *PD_CFG2 =  (unsigned int *)0x2000098;
     volatile unsigned int *PD_DATA  = (unsigned int *)0x20000a0;

    // Below wipes functions for neighbor gpios in this config register
    // but that's okay, because this is a dead-end routine.
    *PD_CFG2 = 0x100;  // configure PD18 for output

    while (1) {
        *PD_DATA ^= 0x40000;    // invert PD18
        for (int c = 0x2f00000; c != 0; c--) ;  // pause
    }
}

// The count_bits function is intended to return the count of
// on bits in a numeric value. But oops-- the code as
// written below is buggy!  A program this simple we could try to
// validate "by inspection" but instead we will demonstrate
// how to find the problem using rudimentary unit tests built on
// the assert macro defined above.
//
// You assert a condition that must be true and if the expression
// evaluates to false, assert reports an error.
// We use the blue ACT LED on the Pi as a status indicator.
// When an assert fails, it calls abort(), which stops
// and flashes the ACT LED. If the program successfully finishes
// executing (no assert failed), the ACT LED turns off.
// Thus the flashing blue LED tells you that your
// program is failing at least one test.

int count_bits(unsigned int val)
{
    int count = 0;

    while (val != 0) {
        val = val >> 1;   // BUGGY: should test/count lsb first, then shift
        if (val & 1) {    // which inputs are affected by this bug?
            count++;
        }
    }
    return count;
}


void main(void) {

    assert(count_bits(0) == 0);
    assert(count_bits(8) == 1);
    assert(count_bits(6) == 2);
    assert(count_bits(7) == 3);
    assert(count_bits(0xf0) == 4);
    assert(count_bits(0x107e) == 7);
    assert(count_bits(-1) == 32);

    // this last test case is mis-constructed
    // it will "pass" on the buggy code for `count_bits`
    // and "fail" after you correct the function
    // It is here as a reminder that your test cases
    // are only truthful if they are correctly constructed!
    assert(count_bits(5) == 1);   // this test case is mis-constructed

	// read cstart.c to find out what happens after main() finishes
}

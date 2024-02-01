/* File: printf.c
 * --------------
 * TODO: add your file header comment here
 */
#include "printf.h"
#include <stdarg.h>
#include <stdint.h>
#include "strings.h"

/* Prototypes for internal helpers.
 * Typically these would be qualified as static (private to module)
 * but, in order to call them from the test program, we declare them externally
 */
int unsigned_to_base(char *buf,
                     size_t bufsize,
                     unsigned long val,
                     int base, size_t
                     min_width);
int signed_to_base(char *buf,
                   size_t bufsize,
                   long val,
                   int base,
                   size_t min_width);


#define MAX_OUTPUT_LEN 1024

int unsigned_to_base(char *buf, size_t bufsize, unsigned long val, int base, size_t min_width) {
    /* TODO: Your code here */
    return 0;
}

int signed_to_base(char *buf, size_t bufsize, long val, int base, size_t min_width) {
    /* TODO: Your code here */
    return 0;
}

int vsnprintf(char *buf, size_t bufsize, const char *format, va_list args) {
    /* TODO: Your code here */
    return 0;
}

int snprintf(char *buf, size_t bufsize, const char *format, ...) {
    /* TODO: Your code here */
    return 0;
}

int printf(const char *format, ...) {
    /* TODO: Your code here */
    return 0;
}


/* From here to end of file is some sample code and suggested approach
 * for those of you doing the disassemble extension. Otherwise, ignore!
 *
 * The struct insn bitfield is declared using exact same layout as bits are organized in
 * the encoded instruction. Accessing struct.field will extract just the bits
 * apportioned to that field. If you look at the assembly the compiler generates
 * to access a bitfield, you will see it simply masks/shifts for you. Neat!
 */
/*
static const char *reg_names[32] = {"zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
                                    "s0/fp", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
                                    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
                                    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6" };

struct insn  {
    uint32_t opcode: 7;
    uint32_t reg_d:  5;
    uint32_t funct3: 3;
    uint32_t reg_s1: 5;
    uint32_t reg_s2: 5;
    uint32_t funct7: 7;
};

void sample_use(unsigned int *addr) {
    struct insn in = *(struct insn *)addr;
    printf("opcode is 0x%x, reg_dst is %s\n", in.opcode, reg_names[in.reg_d]);
}
*/

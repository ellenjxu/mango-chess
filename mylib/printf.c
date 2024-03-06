/* File: printf.c
 * --------------
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 * Simplified printf library for Mango Pi.
 */
#include "printf.h"
#include <stdarg.h>
#include <stdint.h>
#include "strings.h"
#include "uart.h"

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

#define R_OPCODE 0b0110011
#define I_ARITHMETIC_OPCODE 0b0010011
#define I_RV64I_ARITHMETIC_OPCODE 0b0011011
#define R_RV64I_ARITHMETIC_OPCODE 0b0111011
#define I_LOAD_OPCODE 0b0000011
#define I_JALR_OPCODE 0b1100111
#define S_OPCODE 0b0100011
#define B_OPCODE 0b1100011

static const char *reg_names[32] = {"zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
                                    "s0/fp", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
                                    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
                                    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6" };

// r-type instructions utils

// when funct7 is 0x00, index corresponds to value of funct3
static const char *r_funct7_00[8] = {"add", "sll", "slt", "sltu", "xor", "srl", "or", "and"};

// when funct7 is 0x01, index corresponds to value of funct3 (multiply extension!)
static const char *r_funct7_01[8] = {"mul", "mulh", "mulsu", "mulu", "div", "divu", "rem", "remu"};

// special cases when funct7 is 0x02, corresponding values of funct3
#define R_SUB_FUNCT3 0x0
#define R_SRA_FUNCT3 0x5

// i-type instructions utils (empty = special case or invalid)
static const char *i_artihmetic[8] = {"addi", "slli", "slti", "sltiu", "xori", "", "ori", "andi"};
static const char *i_load[8] = {"lb", "lh", "lw", "ld", "lbu", "lhu", "lwu", ""};

// s-type instructions utils
static const char *s_funct3[8] = {"sb", "sh", "sw", "sd", "", "", "", ""};

// b-type instructions utils
static const char *b_funct3[8] = {"beq", "bne", "", "", "blt", "bge", "bltu", "bgeu"};
#define SHIFT_AMOUNT (sizeof(int) * 8 - 13)
#define DIVIDE_AMOUNT (1 << SHIFT_AMOUNT)

struct r_instruction {
    uint32_t opcode: 7;
    uint32_t reg_d:  5;
    uint32_t funct3: 3;
    uint32_t reg_s1: 5;
    uint32_t reg_s2: 5;
    uint32_t funct7: 7;
};

struct i_instruction {
    uint32_t opcode: 7;
    uint32_t reg_d:  5;
    uint32_t funct3: 3;
    uint32_t reg_s1: 5;
    uint32_t imm: 12;
};

struct s_instruction {
    uint32_t opcode: 7;
    uint32_t imm_lo: 5;
    uint32_t funct3: 3;
    uint32_t reg_s1: 5;
    uint32_t reg_s2: 5;
    uint32_t imm_hi: 7;
};

struct b_instruction {
    uint32_t opcode: 7;
    uint32_t imm_11: 1;
    uint32_t imm_1_4: 4;
    uint32_t funct3: 3;
    uint32_t reg_s1: 5;
    uint32_t reg_s2: 5;
    uint32_t imm_5_10: 6;
    uint32_t imm_12: 1;
};

/*
 * Rearrange_buffer fixes the chaotic way things are ordered in unsigned_to_base
 * to avoid limiting the number of characters.
*/
static void rearrange_buffer(char *buf, size_t bufsize, int i) {
    // catch edge cases
    if (bufsize == 0) return; 
    if (bufsize == 1) {
        buf[0] = '\0'; // null terminate
        return;
    }

    // if we have written more than bufsize characters, we need to rearrange
    int last = (i % (bufsize - 1)) - 1; // last character written

    // swap characters in buffer up to last
    for (int j = 0; j <= last / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[last - j];
        buf[last - j] = tmp;
    }

    if (i >= bufsize) { // if there was overflow, rearrange the second half as well
        // first character we did not rearrange above
        int first = last + 1;
        
        // last character, excluding null terminator
        last = bufsize - 2;

        // swap characters in buffer from first to last
        for (int j = 0; j + first <= last / 2; j++) {
            char tmp = buf[first + j];
            buf[first + j] = buf[first + last - j];
            buf[first + last - j] = tmp;
        }

        // we have used the whole buffer, so null terminate the last byte
        buf[bufsize - 1] = '\0';
    } else { // no overflow, simply set null character
        buf[i] = '\0';
    } 
}

int unsigned_to_base(char *buf, size_t bufsize, unsigned long val, int base, size_t min_width) {
    // keep track of how many characters we have written or would have written
    int i = 0;

    // if val=0, ensure we write at least one 0
    if (val == 0 && min_width == 0) min_width = 1;

    // keep dividing by base until the number is obliterated 
    while (val != 0) {
        int digit = val % base;
        if (bufsize > 1) { // avoid divide by 0 and underflow
            // add to buffer in reverse order, overwriting digits if need be
            // and wrapping around. this allows us to avoid using a separate
            // buffer to store the number and then reverse it, which would limit
            // the maximum number of characters we can write
            if (digit < 10) {
                buf[i % (bufsize - 1)] = '0' + digit;
            } else {
                buf[i % (bufsize - 1)] = 'a' + digit - 10;
            }
        }

        i++;
        val /= base;
    }

    // add padding zeros
    while (i < min_width) {
        if (bufsize > 1) buf[i % (bufsize - 1)] = '0';
        i++;
    }

    // rearrange buffer bytes
    rearrange_buffer(buf, bufsize, i);

    // return i, which is the total number of characters written or that would have been written
    return i;
}

int signed_to_base(char *buf, size_t bufsize, long val, int base, size_t min_width) {
    if (val >= 0 || bufsize == 0) {
        // if val is positive, just call unsigned_to_base
        return unsigned_to_base(buf, bufsize, val, base, min_width);
    }
    // if val is negative, write '-' and then unsigned_to_base
    buf[0] = '-';
    return unsigned_to_base(buf + 1, bufsize - 1, -val, base, min_width == 0 ? 0 : min_width - 1) + 1;
}

/*
* Take a buffer (with its corresponding size) and a pointer to an instruction
* and decode the instruction into a string. The buffer will be null-terminated.
* If the buffer is too small, the function will write as many characters as it
* can. Return 1 if the instruction could be decoded and 0 otherwise.
*/
int decode_instruction(char *buf, size_t bufsize, unsigned int *addr) {
    if (bufsize == 0) return 0;
    char largebuf[MAX_OUTPUT_LEN];

    // extract opcode
    unsigned int opcode = *addr & 0b1111111;

    if (opcode == R_OPCODE) {
        struct r_instruction in = *(struct r_instruction *)addr;
        const char *rd = reg_names[in.reg_d];
        const char *rs1 = reg_names[in.reg_s1];
        const char *rs2 = reg_names[in.reg_s2];
        if (in.funct7 == 0x00) {
            snprintf(largebuf, sizeof(largebuf), "%s %s, %s, %s", r_funct7_00[in.funct3], rd, rs1, rs2);
        } else if (in.funct7 == 0x01) {
            snprintf(largebuf, sizeof(largebuf), "%s %s, %s, %s", r_funct7_01[in.funct3], rd, rs1, rs2);
        } else if (in.funct7 == 0x02) {
            if (in.funct3 == R_SUB_FUNCT3) {
                snprintf(largebuf, sizeof(largebuf), "sub %s, %s, %s", rd, rs1, rs2);
            } else if (in.funct3 == R_SRA_FUNCT3) {
                snprintf(largebuf, sizeof(largebuf), "sra %s, %s, %s", rd, rs1, rs2);
            } else { // invalid case
                return 0;
            }
        }
    } else if (opcode == I_ARITHMETIC_OPCODE) {
        struct i_instruction in = *(struct i_instruction *)addr;
        const char *rd = reg_names[in.reg_d];
        const char *rs1 = reg_names[in.reg_s1];

        if (in.funct3 == 0x1 && in.imm >> 5 != 0x00) return 0; // invalid case

        if (in.funct3 == 0x5) { // special case
            if (in.imm >> 5 == 0x00) {
                snprintf(largebuf, sizeof(largebuf), "srli %s, %s, %d", rd, rs1, (int)in.imm);
            } else if (in.imm >> 5 == 0x20) {
                snprintf(largebuf, sizeof(largebuf), "srai %s, %s, %d", rd, rs1, (int)in.imm & 0b11111);
            } else { // invalid case
                return 0;
            }
        } else {
            snprintf(largebuf, sizeof(largebuf), "%s %s, %s, %d", i_artihmetic[in.funct3], rd, rs1, (int)in.imm);
        }
    } else if (opcode == I_RV64I_ARITHMETIC_OPCODE) {
        struct i_instruction in = *(struct i_instruction *)addr;
        const char *rd = reg_names[in.reg_d];
        const char *rs1 = reg_names[in.reg_s1];
        if (in.funct3 == 0x0) {
            snprintf(largebuf, sizeof(largebuf), "addiw %s, %s, %d", rd, rs1, (int)in.imm);
        } else if (in.funct3 == 0x1) {
            snprintf(largebuf, sizeof(largebuf), "slliw %s, %s, %d", rd, rs1, (int)in.imm);
        } else if (in.funct3 == 0x5) {
            if (in.imm >> 5 == 0x20) {
                snprintf(largebuf, sizeof(largebuf), "sraiw %s, %s, %d", rd, rs1, (int)in.imm & 0b11111);
            } else if (in.imm >> 5 == 0x00) {
                snprintf(largebuf, sizeof(largebuf), "srliw %s, %s, %d", rd, rs1, (int)in.imm);
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    } else if (opcode == R_RV64I_ARITHMETIC_OPCODE) {
        struct r_instruction in = *(struct r_instruction *)addr;
        const char *rd = reg_names[in.reg_d];
        const char *rs1 = reg_names[in.reg_s1];
        const char *rs2 = reg_names[in.reg_s2];

        if (in.funct3 == 0x1) {
            snprintf(largebuf, sizeof(largebuf), "sllw %s, %s, %s", rd, rs1, rs2);
        } else if (in.funct3 == 0x0) {
            if (in.funct7 == 0x00) {
                snprintf(largebuf, sizeof(largebuf), "addw %s, %s, %s", rd, rs1, rs2);
            } else if (in.funct7 == 0x20) {
                snprintf(largebuf, sizeof(largebuf), "subw %s, %s, %s", rd, rs1, rs2);
            } else {
                return 0;
            }
        } else if (in.funct3 == 0x5) {
            if (in.funct7 == 0x00) {
                snprintf(largebuf, sizeof(largebuf), "srlw %s, %s, %s", rd, rs1, rs2);
            } else if (in.funct7 == 0x20) {
                snprintf(largebuf, sizeof(largebuf), "sraw %s, %s, %s", rd, rs1, rs2);
            } else {
                return 0;
            }
        } else {
            return 0;
        }

    } else if (opcode == I_LOAD_OPCODE) {
        struct i_instruction in = *(struct i_instruction *)addr;
        const char *rd = reg_names[in.reg_d];
        const char *rs1 = reg_names[in.reg_s1];
        const char *instruction = i_load[in.funct3];

        if (instruction[0] == '\0') return 0; // invalid case

        snprintf(largebuf, sizeof(largebuf), "%s %s, %d(%s)", instruction, rd, (int)in.imm, rs1);
    } else if (opcode == I_JALR_OPCODE) {
        struct i_instruction in = *(struct i_instruction *)addr;
        const char *rd = reg_names[in.reg_d];
        const char *rs1 = reg_names[in.reg_s1];

        if (in.funct3 != 0x00) return 0; // invalid case

        snprintf(largebuf, sizeof(largebuf), "jalr %s, %d(%s)", rd, (int)in.imm, rs1);
    } else if (opcode == S_OPCODE) {
        struct s_instruction in = *(struct s_instruction *)addr;
        const char *rs1 = reg_names[in.reg_s1];
        const char *rs2 = reg_names[in.reg_s2];
        const char *instruction = s_funct3[in.funct3];

        unsigned int immediate = in.imm_lo | (in.imm_hi << 5);

        if (instruction[0] == '\0') return 0; // invalid case

        snprintf(largebuf, sizeof(largebuf), "%s %s, %d(%s)", instruction, rs2, (int)immediate, rs1);
    } else if (opcode == B_OPCODE) {
        struct b_instruction in = *(struct b_instruction *)addr;
        const char *rs1 = reg_names[in.reg_s1];
        const char *rs2 = reg_names[in.reg_s2];
        const char *instruction = b_funct3[in.funct3];

        if (instruction[0] == '\0') return 0; // invalid case

        // crazy bit manipulation to get the immediate
        // the idea is this: we have a 13-bit immediate, but we need to sign-extend it
        // to 32 bits. we do this by shifting the bits to the left 19 times, then dividing
        // by 2^19 to get the sign-extended value.
        unsigned int immediate = ((in.imm_11 << 11) | (in.imm_5_10 << 5) | (in.imm_1_4 << 1) | (in.imm_12 << 12)) << SHIFT_AMOUNT;
        int signed_immediate = *(int *)&immediate / DIVIDE_AMOUNT;

        snprintf(largebuf, sizeof(largebuf), "%s %s, %s, %d", instruction, rs1, rs2, (int)signed_immediate);
    } else {
        return 0;
    }

    int i = 0;
    do {
        buf[i] = largebuf[i];
    } while (++i < bufsize - 1);
    buf[i] = '\0';

    return 1;
}

int vsnprintf(char *buf, size_t _bufsize, const char *format, va_list args) {
    // keeps track of number of characters written or that would have been written
    int i = 0;

    int bufsize = _bufsize; // use integer to make it signed

    // since many string functions rely on null-terminated strings, we need to
    // ensure that the buffer is null-terminated at all times
    if (bufsize > 0) buf[0] = '\0';

    while (*format != '\0') {
        if (*format == '%') { // flag found!
            format++;

            // compute the minimum width if it is specified
            unsigned long min_width = 0;
            if (*format == '0') { // min width specified
                min_width = strtonum(format, &format);
            }
            
            // we use if/else instead of switch to avoid redeclaring variables
            if (*format == '%') { // escape %
                if (i < bufsize - 1) { // avoid buffer overflow
                    buf[i] = '%';
                    buf[i + 1] = '\0';
                }
                i++;

            } else if (*format == 'c') { // character
                char ch = (char) va_arg(args, int);
                if (i < bufsize - 1) { // avoid buffer overflow
                    buf[i] = ch;
                    buf[i + 1] = '\0';
                }
                i++;

            } else if (*format == 'p') { // pointer
                void *ptr = va_arg(args, void *); // retrieve pointer
                char numberbuf[MAX_OUTPUT_LEN]; // set up buffer


                if (format[1] == 'I') {
                    format++;
                    if (!decode_instruction(numberbuf, sizeof(numberbuf), (unsigned int *)ptr)) {
                        // invalid instruction: write value
                        unsigned_to_base(numberbuf, sizeof(numberbuf), *((unsigned int *)ptr), 16, min_width); 
                    }
                } else {
                    // write '0x'
                    numberbuf[0] = '0';
                    numberbuf[1] = 'x';

                    // convert pointer to hex (pointers are always unsigned)
                    unsigned_to_base(numberbuf + 2, sizeof(numberbuf) - 2, (long) ptr, 16, 8);

                }

                // if we call strlcat when i is greater than the buffer
                // (which means we are already truncating and simply counting
                // the number of characters that would have been written),
                // the value of i is not updated properly, so we use strlen
                if (i <= bufsize - 1) {
                    i = strlcat(buf, numberbuf, bufsize);
                } else {
                    i += strlen(numberbuf);
                }

            } else if (*format == 's') { // string
                char *string = va_arg(args, char *); // retrieve string

                // see similar code above
                if (i <= bufsize - 1) {
                    i = strlcat(buf, string, bufsize);
                } else {
                    i += strlen(string);
                }

            } else if (*format == 'l') { // long (don't know hex or decimal so far)
                format++;

                if (*format == 'x') { // long hex
                    unsigned long number = va_arg(args, unsigned long); // retrieve number
                    char numberbuf[MAX_OUTPUT_LEN]; // set up buffer

                    // convert number to hex (hex is always unsigned)
                    unsigned_to_base(numberbuf, sizeof(numberbuf), number, 16, min_width);

                    // see similar code above
                    if (i <= bufsize - 1) {
                        i = strlcat(buf, numberbuf, bufsize);
                    } else {
                        i += strlen(numberbuf);
                    }

                } else if (*format == 'd') { // long decimal
                    long number = va_arg(args, long); // retrieve number
                    char numberbuf[MAX_OUTPUT_LEN]; // set up buffer

                    // convert number to decimal
                    signed_to_base(numberbuf, sizeof(numberbuf), number, 10, min_width);

                    // see similar code above
                    if (i <= bufsize - 1) {
                        i = strlcat(buf, numberbuf, bufsize);
                    } else {
                        i += strlen(numberbuf);
                    }
                }

            } else if (*format == 'd') { // decimal
                long number = (long) va_arg(args, int); // retrieve number
                char numberbuf[MAX_OUTPUT_LEN]; // set up buffer

                // convert number to decimal
                signed_to_base(numberbuf, sizeof(numberbuf), number, 10, min_width);

                // see similar code above
                if (i <= bufsize - 1) {
                    i = strlcat(buf, numberbuf, bufsize);
                } else {
                    i += strlen(numberbuf);
                }

            } else if (*format == 'x') { // hex
                unsigned long number = (unsigned long) va_arg(args, unsigned int); // retrieve number
                char numberbuf[MAX_OUTPUT_LEN]; // set up buffer

                // convert number to hex (hex is always unsigned)
                unsigned_to_base(numberbuf, sizeof(numberbuf), number, 16, min_width);

                // see similar code above
                if (i <= bufsize - 1) {
                    i = strlcat(buf, numberbuf, bufsize);
                } else {
                    i += strlen(numberbuf);
                }
            }

        } else {
            // just add next character
            if (i < bufsize - 1) { // avoid buffer overflow
                buf[i] = *format;
                buf[i + 1] = '\0';
            }
            i++;
        }

        // look at next character
        format++;
    }

    // return i, which is the total number of characters written
    return i;
}

int snprintf(char *buf, size_t bufsize, const char *format, ...) {
    // set up argument list
    va_list args;
    va_start(args, format);

    // call vsnprintf to do the heavy lifting
    int output = vsnprintf(buf, bufsize, format, args);

    // clean up argument list
    va_end(args);

    // return number of characters written
    return output;
}

int printf(const char *format, ...) {
    // assume that the buffer is large enough to hold the output
    char buf[MAX_OUTPUT_LEN];

    // set up argument list
    va_list args;
    va_start(args, format);

    // call vsnprintf to do the heavy lifting
    int output = vsnprintf(buf, sizeof(buf), format, args);

    // clean up argument list
    va_end(args);

    // print the buffer to the UART
    uart_putstring(buf);

    // return number of characters printed
    return output;
}

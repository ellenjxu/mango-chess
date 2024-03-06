/*
 * File: symtab.c
 * ---------------
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 * TODO (extension)
 */

#include "printf.h"
#include "symtab.h"

/*
* The provided code for the symtab module supplies the basic label
* for an address, which is simply the offset of that address within
* the text section, e.g. <.text+24>
*
* If you choose to implement the extension you will re-implement
* these functions to retrieve symbol by name or address and supply
* the fancy label <name+offset>.
*/

extern void _start(void);

static bool is_within_text_section(uintptr_t addr) {
    extern unsigned char __text_end;
    return (addr >= (uintptr_t)_start && addr <= (uintptr_t)&__text_end);
}

bool symtab_symbol_for_name(const char *name, symbol_t *p_symbol) {
    /***** TODO EXTENSION: Your code goes here if implementing extension *****/
    return false;
}

bool symtab_symbol_for_addr(uintptr_t addr, symbol_t *p_symbol) {
    /***** TODO EXTENSION: Your code goes here if implementing extension *****/
    return false;
}

void symtab_label_for_addr(char *buf, size_t bufsize, uintptr_t addr) {
    /***** TODO EXTENSION: Replace with your code if implementing extension *****/
    if (!is_within_text_section(addr)) {
        snprintf(buf, bufsize, "<>");
    } else {
        int offset = addr - (uintptr_t)_start;
        snprintf(buf, bufsize, "<%s+%d>", ".text", offset);
    }
}

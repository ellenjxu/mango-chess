#include "assert.h"
#include <stddef.h>
#include "uart.h"

// --------  functions to implement -------------------

static size_t strlen(const char *str) {
    int n = 0;
    while (str[n] != '\0') {
        n++;
    }
    return n;
}

static char *strcpy(char *dst, const char *src) {
    // TODO: Your turn -- implement strcpy!
    return dst;
}


// --------  unit tests from here down -------------------

void test_strlen(void) {
    char *fruit = "watermelon";

    assert(strlen("green") == 5);
    assert(strlen("") ==  0);
    assert(strlen(fruit) == 2 + strlen(fruit + 2));
}

void test_strcpy(const char *orig) {
    int len = strlen(orig);
    char buf[len + 1]; // plus one for terminator

    char *copy = strcpy(buf, orig);
    assert(copy != orig);
    for (int i = 0; i <= len; i++) // compare letter by letter
        assert(copy[i] == orig[i]);
}

int bogus_strlen_uninitialized(void) {
    char uninitialized[10];
    // what "should" happen on this call? what *does* happen?
    return strlen(uninitialized); // bogus #1
}

int bogus_strlen_no_terminator(void) {
    int neighbor[2] = {0x12345678, 0x12345678};
    char no_terminator[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    // what "should" happen on this call? what *does* happen?
    return strlen(no_terminator); // bogus #2
}

int bogus_strlen_null_ptr(void) {
    // what "should" happen on this call? what *does* happen?
    return strlen(NULL); // bogus #3
}

void main(void) {
    uart_init();

    test_strlen();
    test_strcpy("CS107e rocks"); // uncomment this test after you have implemented strcpy

    // below are tests that make wrong-headed call to strlen
    // uncomment these one by one and run to see what the consequences
    // of these calls
    //assert(bogus_strlen_uninitialized() == 0);
    //assert(bogus_strlen_no_terminator() == 8);
    //assert(bogus_strlen_null_ptr() == 0);
}

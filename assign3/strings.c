/* File: strings.c
 * ---------------
 * TODO: add your file header comment here
 */
#include "strings.h"

void *memcpy(void *dst, const void *src, size_t n) {
    /* Copy contents from src to dst one byte at a time */
    char *d = dst;
    const char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

void *memset(void *dst, int val, size_t n) {
    /* TODO: Your code here */
    return NULL;
}

size_t strlen(const char *str) {
    /* Implementation a gift to you from lab3 */
    size_t n = 0;
    while (str[n] != '\0') {
        n++;
    }
    return n;
}

int strcmp(const char *s1, const char *s2) {
    /* TODO: Your code here */
    return 0;
}

size_t strlcat(char *dst, const char *src, size_t dstsize) {
    /* TODO: Your code here */
    return 0;
}

unsigned long strtonum(const char *str, const char **endptr) {
    /* TODO: Your code here */
    return 0;
}

/* File: strings.c
 * ---------------
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 * Lightweight, simplified C strings library for Mango Pi.
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
    // drop the high bytes of val
    unsigned char valch = (unsigned char) val;

    // write n times
    for (int i = 0; i < n; i++) {
        // cast to char* to allow pointer arithmetic and set value
        ((unsigned char *)dst)[i] = valch;
    }

    return dst;
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
    // loop until we hit a null character
    while (*s1 != '\0' && *s2 != '\0') {
        // if different, stop
        if (*s1 != *s2) return *s1 - *s2;

        // move pointer to next char
        s1++;
        s2++;
    }

    // subtract to catch case where one string terminates first
    return *s1 - *s2;
}

size_t strlcat(char *dst, const char *src, size_t dstsize) {
    int i = 0;
    int found = -1;

    // find null character in dst
    do {
        if (dst[i] == '\0') {
            found = i;
            break;
        }
    } while (++i < dstsize);
    
    int srclen = strlen(src);

    // dstsize incorrect or dst not a valid string: no action
    if (found == -1) {
        return dstsize + srclen;
    }

    // concatenate string
    if (found + srclen + 1 <= dstsize) {
        memcpy(dst + found, src, srclen + 1);
    } else {
        memcpy(dst + found, src, dstsize - found - 1);
        dst[dstsize - 1] = '\0';
    }

    return found + srclen;
}

unsigned long strtonum(const char *str, const char **endptr) {
    // compute base
    int base = str[0] == '0' && str[1] == 'x' ? 16 : 10;

    // skip '0x' characters
    if (base == 16) str += 2;

    unsigned long result = 0;
    while (1) {
        if ('0' <= *str && *str <= '9') { // 0-9
            result *= base;
            result += *str - '0';
        } else if (base == 16 && 'a' <= *str && *str <= 'f') { // a-f
            result *= base;
            result += *str - 'a' + 10;
        } else if (base == 16 && 'A' <= *str && *str <= 'F') { // A-F
            result *= base;
            result += *str - 'A' + 10;
        } else {
            // we will end up here at the first invalid character (including \0)
            if (endptr != NULL) *endptr = str;
            break;
        }

        // move to next character
        str++;
    }

    return result;
}

#include "printf.h"
#include "uart.h"

void main(void) {
    uart_init();
    int factor = gcd(144, 32);
    printf("gcd %d\n", factor);
}

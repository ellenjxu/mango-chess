#include "uart.h"
#include "mymodule.h"

void main(void) {
    uart_init();
    say_hello("CS107e");
}

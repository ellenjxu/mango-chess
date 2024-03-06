#include "uart.h"
#include "re.h"
#include "mymodule.h"

void main(void) {
    uart_init();
    say_hello("CS107e");
    re_test();
}

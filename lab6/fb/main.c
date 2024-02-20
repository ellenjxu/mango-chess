#include "fb.h"
#include "uart.h"
#include "printf.h"

void main(void) {
    uart_init();
    printf("\nStarting %s\n", __FILE__);
    fb_init(640, 480, FB_SINGLEBUFFER);
    printf("Frambuffer initialized to %d x %d\n", 640, 480);

    printf("Hit any key to quit: ");
    uart_getchar();
    printf("\nCompleted %s\n", __FILE__);
}

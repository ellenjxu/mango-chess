
void main(void) {
    uart_init();

    uart_putstring("Hello, hola, ciao, tere\n");    // How can such different calls to uart_putstring all be "ok"?
    uart_putstring();
    uart_putstring(1, 4, 6);
    uart_putstring(main);
    uart_putstring("BYE!");
}

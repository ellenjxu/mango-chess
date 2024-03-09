/*
 * Module to control HM-10 Bluetooth module.
 *
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 * Uart code adapted from provided uart.c module (by Julie Zelenski).
 */
#include "bt_ext.h"
#include "ccu.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "interrupts.h"
#include "ringbuffer.h"
#include "ringbuffer_ptr.h"
#include "timer.h"
#include <stdint.h>
#include <stddef.h>

#define UART_TX GPIO_PB2
#define UART_RX GPIO_PB3
#define UART_FN GPIO_FN_ALT7
#define UART_INDEX 4

#define RECEIVING_TIMEOUT_USEC 10 * 1000 // 10 ms, time for 12 bytes
#define WAITING_TIMEOUT_USEC 3000 * 1000 // 1 s, time for AT response

#define LCR_DLAB            (1 << 7)
#define USR_BUSY            (1 << 0)
#define USR_TX_NOT_FULL     (1 << 1)
#define USR_TX_NOT_EMPTY    (1 << 2)
#define USR_RX_NOT_EMPTY    (1 << 3)

// structs defined to match layout of hardware registers
typedef union {
    struct {
        union {
            uint32_t rbr;   // receive buffer register
            uint32_t thr;   // transmit holding register
            uint32_t dll;   // divisor latch (LSB)
        };
        union {
            uint32_t dlh;   // divisor latch (MSB)
            uint32_t ier;   // interrupt enable register
        };
        union {
            uint32_t iir;   // interrupt identification register
            uint32_t fcr;   // FIFO control register
        };
        uint32_t lcr;       // line control register
        uint32_t mcr;       // modem control register
        uint32_t lsr;       // line status register
        uint32_t reserved[25];
        uint32_t usr;       // busy status, at offset 0x7c
        uint32_t reserved2[9];
        uint32_t halt;      // at offset 0xa4
    } regs;
    unsigned char padding[0x400];
} uart_t;

#define UART_BASE ((uart_t *)0x02500000)

typedef enum {
    IDLE,       // not waiting for AT response
    WAITING,    // waiting for AT response
    RECEIVING,  // receiving AT response
} state_t;

static struct {
    volatile uart_t *uart;

    rb_t *rb_rx;
    rb_ptr_t *rb_tx;

    bt_ext_role_t role;

    state_t state;
    volatile unsigned long last_rx;
} module;

static void handler_rx(uintptr_t pc, void *data) {
    if (module.state == WAITING) {
        module.state = RECEIVING;
    }

    unsigned char byte = module.uart->regs.rbr & 0xFF;
    rb_enqueue(module.rb_rx, byte);

    unsigned long now = timer_get_ticks();
    module.last_rx = now;
}

static void setup_uart() {
    module.uart = UART_BASE + UART_INDEX;

    // clock up peripheral
    // gating bits [0:5], reset bits [16:21]
    uint32_t bit = 1 << UART_INDEX;
    uint32_t reset = bit << 16;
    ccu_enable_bus_clk(CCU_UART_BGR_REG, bit, reset);

    // configure GPIOs
    gpio_set_function(UART_TX, UART_FN);
    gpio_set_pullup(UART_TX);
    gpio_set_function(UART_RX, UART_FN);
    gpio_set_pullup(UART_RX);

    // configure baud rate
    uint32_t baud = 9600;
    module.uart->regs.fcr = 1;      // enable TX/RX fifo
    module.uart->regs.halt = 1;     // temporarily disable TX transfer

    uint32_t sys_clock_rate = 24 * 1000000;
    uint32_t udiv = sys_clock_rate / (16 * baud);
    module.uart->regs.lcr |= LCR_DLAB;  // set DLAB = 1 to access DLL/DLH
    module.uart->regs.dll = udiv & 0xff;        // low byte of divisor -> DLL
    module.uart->regs.dlh = (udiv >> 8) & 0xff; // hi byte of divisor -> DLH
    module.uart->regs.lcr &= ~LCR_DLAB; // set DLAB = 0 to access RBR/THR
    module.uart->regs.halt = 0;     // re-enable TX transfer

    // configure data-parity-stop (low 4 bits of LCR)
    uint8_t data = 0b11;    // 8 data
    uint8_t parity = 0b0;   // no parity
    uint8_t stop = 0b0;     // 1 stop
    uint8_t settings = (parity << 3) | (stop << 2) | (data << 0);
    // clear low 4 bits, replace with settings 8-n-1
    module.uart->regs.lcr = (module.uart->regs.lcr & ~0b1111) | settings;

    module.uart->regs.mcr = 0;    // disable modem control
    module.uart->regs.ier = 0;    // disable interrupts

    // interrupt_source_t src = INTERRUPT_SOURCE_UART0 + UART_INDEX;
    // interrupts_register_handler(src, handler_rx, NULL); // install handler
    // interrupts_enable_source(src);  // turn on source
    // module.uart->regs.ier = 1;      // enable interrupts in uart peripheral
}

static void send_uart(unsigned char byte) {
    while ((module.uart->regs.usr & USR_TX_NOT_FULL) == 0) ;
    module.uart->regs.thr = byte & 0xFF;
}

static void flush_uart(void) {
    while ((module.uart->regs.usr & USR_BUSY) != 0) ;
}

static bool haschar_uart(void) {
    return (module.uart->regs.usr & USR_RX_NOT_EMPTY) != 0;
}

static unsigned char recv_uart(void) {
    return module.uart->regs.rbr & 0xFF;
}

void bt_ext_init() {
    setup_uart();

    module.rb_rx = rb_new();
    module.rb_tx = rb_ptr_new();
}

static void update_receiving_state(void) {
    if (module.state != RECEIVING)
        return;

    unsigned long now = timer_get_ticks();
    if (now - module.last_rx > RECEIVING_TIMEOUT_USEC * TICKS_PER_USEC) {
        module.state = IDLE;
    }
}

void bt_ext_send(const char *str) {
    while (module.state != IDLE) {
        update_receiving_state();
    }

    while (*str) {
        send_uart(*str++);
    }

    // module.state = WAITING;
}

int bt_ext_read(char *buf, size_t len) {
    for (size_t i = 0; i < len - 1; i++) {
        // int res;
        // if (!rb_dequeue(module.rb_rx, &res)) {
        if (!haschar_uart()) {
            buf[i] = '\0';
            return i;
        }
        unsigned char res = recv_uart();
        buf[i] = res;
    }
    buf[len - 1] = '\0';
    return len;
}

bool bt_has_data() {
    // return !rb_empty(module.rb_rx);
    return haschar_uart();
}


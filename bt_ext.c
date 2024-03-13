/*
 * Module to control HM-10 Bluetooth module.
 *
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 * Uart code adapted from provided uart.c module (by Julie Zelenski).
 */
#include "assert.h"
#include "bt_ext.h"
#include "ccu.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "interrupts.h"
#include "printf.h"
#include "ringbuffer.h"
#include "strings.h"
#include "timer.h"
#include <stdint.h>
#include <stddef.h>

#define UART_TX GPIO_PB2
#define UART_RX GPIO_PB3
#define UART_FN GPIO_FN_ALT7
#define UART_INDEX 4

#define RESPONSE_TIMEOUT_USEC 100 * 1000 // 1000 ms
#define RETRIES               3

#define LCR_DLAB            (1 << 7)
#define USR_BUSY            (1 << 0)
#define USR_TX_NOT_FULL     (1 << 1)
#define USR_TX_NOT_EMPTY    (1 << 2)
#define USR_RX_NOT_EMPTY    (1 << 3)

#define CONNECTED_MESSAGE "OK+CONN"
#define LOST_MESSAGE      "OK+LOST"


#define SIZE(x) ((sizeof(x)) / (sizeof(*x)))

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

static struct {
    volatile uart_t *uart;

    bt_ext_role_t role;
    bool connected;

    rb_t *rxbuf;

    bt_ext_fn_t trigger[256];

    volatile unsigned long last_rx;
} module;

static bool haschar_uart(void) {
    return (module.uart->regs.usr & USR_RX_NOT_EMPTY) != 0;
}

static uint8_t dequeue_byte(void) {
    assert(!rb_empty(module.rxbuf));
    int res_integer;
    rb_dequeue(module.rxbuf, &res_integer);
    uint8_t res = res_integer & 0xFF;
    return res;
}

static bool ringstrcmp(uint8_t *buf, size_t bufsize, int nbytes, const char *cmp, size_t cmplen) {
    if (nbytes < cmplen) return false;

    int base = (nbytes - cmplen) % bufsize;
    for (int i = 0; i < cmplen; i++) {
        if (buf[(base + i) % bufsize] != cmp[i])
            return false;
    }

    return true;
}

static uint8_t recv_uart(void) {
    static uint8_t ring[7] = { '\0' };
    static int nbytes = 32;

    uint8_t byte = module.uart->regs.rbr & 0xFF;

    ring[nbytes % sizeof(ring)] = byte;
    
    if (ringstrcmp(ring, sizeof(ring), nbytes, CONNECTED_MESSAGE, sizeof(CONNECTED_MESSAGE) - 1)) {
        module.connected = true;
    } else if (ringstrcmp(ring, sizeof(ring), nbytes, LOST_MESSAGE, sizeof(LOST_MESSAGE) - 1)) {
        module.connected = false;
    }

    return byte;
}

static void handle_interrupt(uintptr_t pc, void *data) {
    while (haschar_uart()) {
        uint8_t byte = recv_uart();
        int byte_integer = 0xFF & byte;
        rb_enqueue(module.rxbuf, byte);

        if (module.trigger[byte_integer] != NULL) {
            module.trigger[byte_integer]();
        }
    }
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

    interrupt_source_t src = INTERRUPT_SOURCE_UART0 + UART_INDEX;
    interrupts_register_handler(src, handle_interrupt, NULL); // install handler
    interrupts_enable_source(src);  // turn on source
    module.uart->regs.ier = 1;      // enable interrupts in uart peripheral

}

// static void flush_uart(void) {
//     while ((module.uart->regs.usr & USR_BUSY) != 0) ;
// }


void bt_ext_register_trigger(uint8_t byte, bt_ext_fn_t fn) {
    assert(module.trigger[byte] == NULL);
    module.trigger[byte] = fn;
}

void bt_ext_unregister_trigger(uint8_t byte) {
    module.trigger[byte] = NULL;
}

static bool wait_response(uint8_t *buf, size_t len) {
    assert(buf != NULL || len == 0);

    if (len > 0) buf[0] = '\0';

    int nbytes = 0;
    bool ok_response = true;

    unsigned long start = timer_get_ticks();
    while (timer_get_ticks() - start < RESPONSE_TIMEOUT_USEC * TICKS_PER_USEC) {
        if (bt_ext_has_data()) {
            uint8_t byte = dequeue_byte();
            if ((nbytes == 0 && byte != 'O') || (nbytes == 1 && byte != 'K'))
                ok_response = false;

            if (++nbytes < len) {
                buf[nbytes - 1] = byte;
                buf[nbytes] = '\0';
            }
        }
    }

    return nbytes >= 2 && ok_response;
}

void bt_ext_init() {
    static const char *CONFIG_COMMANDS[] = {
        "AT",
        "AT+RESET",
        "AT+NOTI1",
    };
    
    module.rxbuf = rb_new();

    setup_uart();

    for (int i = 0; i < SIZE(CONFIG_COMMANDS); i++) {
        bt_ext_send_cmd(CONFIG_COMMANDS[i], NULL, 0);
    }
}

void bt_ext_send_raw_str(const char *buf) {
    while (*buf) {
        bt_ext_send_raw_byte(*buf++);
    }
}

void bt_ext_send_raw_array(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        bt_ext_send_raw_byte(buf[i]);
    }
}

void bt_ext_send_raw_byte(const uint8_t byte) {
    while ((module.uart->regs.usr & USR_TX_NOT_FULL) == 0) ;
    module.uart->regs.thr = byte;
}

bool bt_ext_send_cmd(const char *str, uint8_t *response, size_t len) {
    if (str == NULL) return false;

    for (int i = 0; i < RETRIES; i++) {
        bt_ext_send_raw_str(str);
        if (wait_response(response, len)) {
            return true;
        }
    }

    return false;
}

void bt_ext_connect(const bt_ext_role_t role, const char *mac) {
    static const char *ROLE_COMMANDS[] = { "AT+ROLE0", "AT+ROLE1" };

    module.role = role;

    bt_ext_send_cmd("AT", NULL, 0);
    bt_ext_send_cmd(ROLE_COMMANDS[role], NULL, 0);

    if (role == BT_EXT_ROLE_PRIMARY) {
        char buf[32];
        buf[0] = '\0';
        strlcat(buf, "AT+CON", sizeof(buf));
        strlcat(buf, mac, sizeof(buf));
        bt_ext_send_cmd(buf, NULL, 0);
    } // SUBORDINATE does not need to connect, just wait
}

int bt_ext_read(uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len - 1; i++) {
        if (!bt_ext_has_data()) {
            buf[i] = '\0';
            return i;
        }
        buf[i] = dequeue_byte();
    }
    buf[len - 1] = '\0';
    return len;
}

bool bt_ext_has_data() {
    return !rb_empty(module.rxbuf);
}

bool bt_ext_connected() {
    return module.connected;
}


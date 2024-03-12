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
#include "strings.h"
#include "timer.h"
#include <stdint.h>
#include <stddef.h>

#define UART_TX GPIO_PB2
#define UART_RX GPIO_PB3
#define UART_FN GPIO_FN_ALT7
#define UART_INDEX 4

#define RESPONSE_TIMEOUT_USEC 10 * 1000 // 10 ms, time for 12 bytes
#define RETRIES               3

#define LCR_DLAB            (1 << 7)
#define USR_BUSY            (1 << 0)
#define USR_TX_NOT_FULL     (1 << 1)
#define USR_TX_NOT_EMPTY    (1 << 2)
#define USR_RX_NOT_EMPTY    (1 << 3)

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

    volatile unsigned long last_rx;
} module;

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

static bool ringstrcmp(char *buf, size_t bufsize, int nbytes, char *cmp, size_t cmplen) {
    if (nbytes < cmplen) return false;

    int base = (nbytes - cmplen) % bufsize;
    for (int i = 0; i < cmplen; i++) {
        if (buf[(base + i) % bufsize] != cmp[i])
            return false;
    }

    return true;
}

static unsigned char recv_uart(void) {
    static const char *CONNECTED_MESSAGE = "OK+CONN";
    static const char *LOST_MESSAGE      = "OK+LOST";

    static char ring[7] = { '\0' };
    static int nbytes = 32;

    unsigned char byte = module.uart->regs.rbr & 0xFF;

    ring[nbytes % sizeof(ring)] = byte;
    
    if (ringstrcmp(ring, sizeof(ring), nbytes, "OK+CONN", 7)) {
        module.connected = true;
    } else if (ringstrcmp(ring, sizeof(ring), nbytes, "OK+LOST", 7)) {
        module.connected = false;
    }

    return byte;
}

static bool wait_response(char *buf, size_t len) {
    assert(buf != NULL || len == 0);

    if (len > 0) buf[0] = '\n';

    int nbytes = 0;
    bool ok_response = true;

    unsigned long start = timer_get_ticks();
    while (timer_get_ticks() - start < RESPONSE_TIMEOUT_USEC * TICKS_PER_USEC) {
        if (haschar_uart()) {
            unsigned char byte = recv_uart();
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

    setup_uart();

    for (int i = 0; i < SIZE(CONFIG_COMMANDS); i++) {
        bt_ext_send(CONFIG_COMMANDS[i], NULL, 0);
    }
}

static void sendstr(const char *str) {
    while (*str) {
        send_uart(*str++);
    }
}

bool bt_ext_send(const char *str, char *response, size_t len) {
    for (int i = 0; i < RETRIES; i++) {
        sendstr(str);
        if (response != NULL) {
            if (wait_response(response, len)) {
                return true;
            }
        }
    }

    return false;
}

void bt_ext_connect(const bt_ext_role_t role, const char *mac) {
    static const char *ROLE_COMMANDS[] = { "AT+ROLE0", "AT+ROLE1" };
    module.role = role;

    bt_ext_send("AT", NULL, 0);
    bt_ext_send(ROLE_COMMANDS[role], NULL, 0);

    if (role == BT_EXT_ROLE_PRIMARY) {
        sendstr("AT+CON");
        bt_ext_send(mac, NULL, 0);
    }
}

int bt_ext_read(char *buf, size_t len) {
    for (size_t i = 0; i < len - 1; i++) {
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

bool bt_ext_has_data() {
    return haschar_uart();
}

bool bt_ext_connected() {
    return module.connected;
}

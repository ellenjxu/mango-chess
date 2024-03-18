/*
 * Module to control HM-10 Bluetooth module over UART.
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
#include "ringbuffer.h"
#include "strings.h"
#include "timer.h"

// #define BT_DEBUG 1

#if BT_DEBUG == 1
#include "printf.h"
#endif

// UART configuration
#define UART_TX GPIO_PB2
#define UART_RX GPIO_PB3
#define UART_FN GPIO_FN_ALT7
#define UART_INDEX 4

#define RESPONSE_TIMEOUT_USEC (100 * 1000) // 100 ms
#define RETRIES               3

#define CONNECTED_MESSAGE_TIMEOUT_USEC (10 * 1000) // 10 ms
#define CONNECTED_MESSAGE "OK+CONN"
#define LOST_MESSAGE      "OK+LOST"

#define ROLE_ENSURE_DELAY_MS 500

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

#define LCR_DLAB            (1 << 7)
#define USR_BUSY            (1 << 0)
#define USR_TX_NOT_FULL     (1 << 1)
#define USR_TX_NOT_EMPTY    (1 << 2)
#define USR_RX_NOT_EMPTY    (1 << 3)

static struct {
    volatile uart_t *uart;

    bt_ext_role_t role;
    volatile bool connected;

    volatile int bytes_since_last_trigger;

    bt_ext_role_t board_role; // role the board is set to currently
    bool role_is_set; // whether the role has been set or not

    rb_t *rxbuf;

    bt_ext_fn_t trigger[256];
    bt_ext_fn_t fallback_trigger;

    volatile unsigned long last_rx;

    bool initialized;
} module;

static struct {
    int nbytes;
    uint8_t buf[32];
} ring;

/*
 * Returns true if the UART has a character to read.
 */
static bool haschar_uart(void) {
    return (module.uart->regs.usr & USR_RX_NOT_EMPTY) != 0;
}

/*
 * Dequeues a byte from the ring buffer. Asserts buffer is not empty. Converts
 * the integer to a byte.
 */
static uint8_t dequeue_byte(void) {
    assert(!rb_empty(module.rxbuf));
    int res_integer;
    rb_dequeue(module.rxbuf, &res_integer);
    uint8_t res = res_integer & 0xFF;
    return res;
}

/*
 * Similar to strcmp but for a ringbuffer. We use uint8_t and char interchangeably
 * to make its use more convenient, but comparisons are bit-for-bit.
 *
 * @param buf       ring buffer
 * @param bufsize   size of the ring buffer
 * @param nbytes    number of bytes in the ring buffer (can be larger than
 *                      bufsize)
 * @param cmp       string to compare
 * @param cmplen    length of the string to compare
 * @return          true if the last cmplen bytes in the ring buffer are equal
 *                      to cmp
 */
static bool ringstrcmp(uint8_t *buf, size_t bufsize, int nbytes, const char *cmp, size_t cmplen) {
    if (nbytes < cmplen) return false;

    int base = (nbytes - cmplen) % bufsize;
    for (int i = 0; i < cmplen; i++) {
        if (buf[(base + i) % bufsize] != (uint8_t)cmp[i])
            return false;
    }

    return true;
}

/*
 * OK+CONNA, OK+CONNE, OK+CONNF each has its own meaning, and do not
 * indicate a connection has been established. The whole reason we need
 * a function to check for OK+CONN but not for OK+LOST it to filter out
 * these cases.
 *
 * If, for example, we receive OK+CONN, and the next character is an 'A',
 * 'E', or 'F', we discard the message and wait for the next one, unless enough
 * time passed between OK+CONN and the next character such that we can assume
 * the message was lost.
 *
 * @return  `true` if the last message received was a connection message.
 */
static bool did_connect(void) {
    uint8_t last = ring.buf[(ring.nbytes - 1) % sizeof(ring.buf)];
    switch (last) {
        case 'A':
        case 'E':
        case 'F':
            if (timer_get_ticks() - module.last_rx < CONNECTED_MESSAGE_TIMEOUT_USEC * TICKS_PER_USEC)
                break;
            // fallthrough
        default:
            // OK+CONN + some other character
            return ringstrcmp(ring.buf, sizeof(ring.buf), ring.nbytes - 1, CONNECTED_MESSAGE, sizeof(CONNECTED_MESSAGE) - 1);
    }
    return false;
}

/*
 * Receives a byte from the UART and stores it in the ring buffer. If the
 * message is a connection message, sets the module's connected flag to true.
 * 
 * @return  the byte received
 */
static uint8_t recv_uart(void) {
    // read the byte from the UART register
    uint8_t byte = module.uart->regs.rbr & 0xFF;

    // store the byte in the cache ring buffer
    ring.buf[ring.nbytes++ % sizeof(ring.buf)] = byte;

    if (did_connect()) {
        module.connected = true;

    // else if lost connection
    } else if (ringstrcmp(ring.buf, sizeof(ring.buf), ring.nbytes, LOST_MESSAGE, sizeof(LOST_MESSAGE) - 1)) {
        module.connected = false;
    }

    // TEST
#if BT_DEBUG == 1
    printf("%c", byte);
#endif

    return byte;
}

/*
 * Handles an interrupt from the UART. If there is a character to read, it
 * reads it and stores it in the ring buffer. If the character is a trigger
 * character, it calls the trigger function. If the character is not a trigger
 * character but the number of bytes since the last trigger is greater than
 * BT_EXT_MAX_BYTES_NO_TRIGGER, it calls the fallback trigger function.
 */
static void handle_interrupt(uintptr_t pc, void *data) {
    while (haschar_uart()) {
        uint8_t byte = recv_uart();
        module.last_rx = timer_get_ticks();

        int byte_integer = 0xFF & byte;
        rb_enqueue(module.rxbuf, byte);

        if (module.trigger[byte_integer] != NULL) {
            // trigger function available, call
            module.trigger[byte_integer]();
            module.bytes_since_last_trigger = 0;
        } else if (module.bytes_since_last_trigger < BT_EXT_MAX_BYTES_NO_TRIGGER) {
            // trigger function not available, increment counter
            module.bytes_since_last_trigger++;
        } else if (module.fallback_trigger != NULL) {
            // fallback trigger function available and need to call, call
            module.fallback_trigger();
            module.bytes_since_last_trigger = 0;
        } // should call fallback trigger function but it's not available, do nothing
    }
}

// In case it's needed in the future.
// static void flush_uart(void) {
//     while ((module.uart->regs.usr & USR_BUSY) != 0) ;
// }

void bt_ext_register_trigger(uint8_t byte, bt_ext_fn_t fn) {
    assert(module.trigger[byte] == NULL);
    module.trigger[byte] = fn;
}

void bt_ext_register_fallback_trigger(bt_ext_fn_t fn) {
    module.fallback_trigger = fn;
}

void bt_ext_unregister_trigger(uint8_t byte) {
    module.trigger[byte] = NULL;
}

/*
 * To be called after sending an AT command. Waits for a response from the module
 * and stores it in the buffer. Returns true if a response starting with "OK"
 * was received and false otherwise. For convenience, the response is terminated
 * with a null character. We make the assumption that the module will not send
 * a \0 character in its response, since AT commands are restricted to ASCII.
 *
 * @param buf   buffer to store the response. Can be NULL if response is not
 *                      needed.
 * @param len   length of the buffer (must be 0 if buf is NULL)
 * @return      true if a response starting with "OK" was received, false
 *              otherwise
 */
static bool wait_response(uint8_t *buf, size_t len) {
    // buffer cannot be NULL if len > 0
    assert(buf != NULL || len == 0);

    // set first byte to \0 if buffer is not NULL
    if (len > 0) buf[0] = '\0';

    int nbytes = 0;
    bool ok_response = true;

    unsigned long start = timer_get_ticks();
    while (timer_get_ticks() - start < RESPONSE_TIMEOUT_USEC * TICKS_PER_USEC) {
        if (bt_ext_has_data()) {
            uint8_t byte = dequeue_byte();

            // keep track that first two bytes are 'O' and 'K'
            if ((nbytes == 0 && byte != 'O') || (nbytes == 1 && byte != 'K'))
                ok_response = false;

            // if there is space, we store the byte in the buffer
            if (++nbytes < len) {
                buf[nbytes - 1] = byte;
                buf[nbytes] = '\0';
            }
        }
    }

    // return true if we received at least 2 bytes and the response was OK
    return nbytes >= 2 && ok_response;
}

bool bt_ext_send_cmd(const char *str, uint8_t *response, size_t len) {
    if (str == NULL) return false;

    // send the command, retrying if necessary
    for (int i = 0; i < RETRIES; i++) {
        bt_ext_send_raw_str(str);
        if (wait_response(response, len)) {
            return true;
        }
    }

    return false;
}

void bt_ext_send_raw_byte(const uint8_t byte) {
    while ((module.uart->regs.usr & USR_TX_NOT_FULL) == 0) ;
    module.uart->regs.thr = byte;

    // TEST
#if BT_DEBUG == 1
    printf("%c", byte);
#endif
}

void bt_ext_send_raw_str(const char *buf) {
    // send until null character
    while (*buf) {
        bt_ext_send_raw_byte(*buf++);
    }
}

void bt_ext_send_raw_array(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        bt_ext_send_raw_byte(buf[i]);
    }
}

/*
 * Checks whether the role is set to the desired role. If not, sets the role.
 *
 * @return  false if, due to a failure, the role is not set to the desired role,
 *              true otherwise.
 */
static bool ensure_role(void) {
    static const char *ROLE_COMMANDS[]  = { "AT+ROLE0", "AT+ROLE1" };
    static const char *ROLE_RESPONSES[] = { "OK+Set:0", "OK+Set:1" };

    // If incorrect role is set, or no role is set, set the role
    if (module.role != module.board_role || !module.role_is_set) {
        // Send AT command to close any open connections
        bt_ext_send_cmd("AT", NULL, 0);

        // Send AT command to set role and store response
        uint8_t response[256];
        bool ret = bt_ext_send_cmd(ROLE_COMMANDS[module.role], response, sizeof(response));

        // After a role command is sent, we need to wait before sending anything
        // else. I have no idea why this is the case, but after three hours of
        // painful debugging, I found that this is the only way to make it work.
        timer_delay_ms(ROLE_ENSURE_DELAY_MS);

        // If the role was set, update the board role and return true
        if (ret && strcmp((char *)response, ROLE_RESPONSES[module.role]) == 0) {
            module.board_role = module.role;
            module.role_is_set = true;
            return true;
        } else { // something went wrong, return false
            return false;
        }
    } else {
        return true;
    }
}

void bt_ext_connect(const bt_ext_role_t role, const char *mac) {
    module.role = role;

    // Set the role if it has not been set. If it fails, return.
    if (!ensure_role())
        return;

    if (module.role == BT_EXT_ROLE_PRIMARY) {
        bt_ext_send_cmd("AT+ERASE", NULL, 0);
        bt_ext_send_cmd("AT+CLEAR", NULL, 0);

        // Send connection command.
        char buf[32]; // 32 is enough for "AT+CON" + 12 (mac) + 1 (\0)

        buf[0] = '\0'; // Set to null character for strlcat

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

bool bt_ext_has_data(void) {
    return !rb_empty(module.rxbuf);
}

bool bt_ext_connected(void) {
    if (!module.connected &&
            timer_get_ticks() - module.last_rx > CONNECTED_MESSAGE_TIMEOUT_USEC * TICKS_PER_USEC &&
            ringstrcmp(ring.buf, sizeof(ring.buf), ring.nbytes, CONNECTED_MESSAGE, sizeof(CONNECTED_MESSAGE) - 1)) {
        module.connected = true;
    }

    return module.connected;
}

// modified from uart.c
static void setup_uart(void) {
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

    // MY ADITION: register interrupt
    interrupt_source_t src = INTERRUPT_SOURCE_UART0 + UART_INDEX;
    interrupts_register_handler(src, handle_interrupt, NULL); // install handler
    interrupts_enable_source(src);  // turn on source
    module.uart->regs.ier = 1;      // enable interrupts in uart peripheral
}

void bt_ext_init(void) {
    if (module.initialized) return;
    module.initialized = true;

    static const char *CONFIG_COMMANDS[] = {
        "AT",       // closes any open connections
        "AT+RESET", // reset module
        "AT+NOTI1", // enable notifications (OK+CONN and OK+LOST)
    };

    module.rxbuf = rb_new();
    setup_uart();

    // initialize the ring buffer and pretend we have received as many bytes as
    // the size of the buffer, which makes the code more robus (i.e. we never
    // have to worry about the special case where the buffer has too few bytes
    // and risk negative values). Besides, we know the bytes in the buffer are
    // all '\0' characters.
    ring.nbytes = sizeof(ring.buf);

    // send config commands.
    for (int i = 0; i < SIZE(CONFIG_COMMANDS); i++) {
        bt_ext_send_cmd(CONFIG_COMMANDS[i], NULL, 0);
    }
}

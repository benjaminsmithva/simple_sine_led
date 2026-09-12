/* 
 * Hardware (STM32F4DISCOVERY):
 *   UART → USART2: PA2 = TX, PA3 = RX, AF7
 *   Clock: HSI 16 MHz (reset default, no PLL)
 *   APB1 = 16 MHz  →  USART2 clock = 16 MHz
 *
 * Compile-time flags:
 *   -DENABLE_UART_TX   enables Part 2
 *   -DENABLE_UART_RX   enables Part 3 (also requires ENABLE_UART_TX)
 */

#define ENABLE_UART_RX
#define ENABLE_UART_TX

#include "stm32f407.h"
#include <stdint.h>
#include <stdbool.h>
#ifdef ENABLE_UART_TX
#include <stdio.h>      /* snprintf from newlib-nano */
#endif

/* ── Constants ──────────────────────────────────────────────────────────── */
#define SYSCLK_HZ       16000000UL
#define SYSTICK_1MS     (SYSCLK_HZ / 1000U - 1U)   /* reload = 15999 */

/* BRR for 115200 baud, 16× oversampling (OVER8=0), APB1 = 16 MHz:
 *   USARTDIV = fCK / (16 × baud) = 16 000 000 / (16 × 115 200) = 8.680
 *   Mantissa = 8  (integer part)          → BRR[15:4]
 *   Fraction = round(0.680 × 16) = 11     → BRR[3:0]
 *   BRR = (8 << 4) | 11 = 0x8B
 *   Actual baud = 16 000 000 / (16 × 8.6875) = 115 108 (<0.1% error) */
#define USART2_BRR_VAL  ((8U << 4) | 11U)

#define UART_TX_PIN     2U      /* PA2  = USART2 TX, AF7 */
#define UART_RX_PIN     3U      /* PA3  = USART2 RX, AF7 */
#define UART_AF         7U

/* Default blink period: 2000 ms total → 0.5 Hz (half-period = 1000 ms) */
#define DEFAULT_PERIOD_MS       2000U
#define MIN_PERIOD_MS           200U
#define MAX_PERIOD_MS           20000U

/* ── UART TX ─────────────────────────────────────────────────────────────
 * Compiled only when ENABLE_UART_TX is defined.
 * ─────────────────────────────────────────────────────────────────────── */
#ifdef ENABLE_UART_TX

static void uart_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)RCC->APB1ENR;

    /* PA2 — TX: AF7, push-pull, high speed */
    GPIOA->MODER   &= ~(0x3U << (UART_TX_PIN * 2));
    GPIOA->MODER   |=  (GPIO_MODER_AF << (UART_TX_PIN * 2));
    GPIOA->OTYPER  &= ~(1U << UART_TX_PIN);
    GPIOA->OSPEEDR |=  (0x2U << (UART_TX_PIN * 2));
    GPIOA->AFR[0]  &= ~(0xFU << (UART_TX_PIN * 4));
    GPIOA->AFR[0]  |=  (UART_AF << (UART_TX_PIN * 4));

#ifdef ENABLE_UART_RX
    /* PA3 — RX: AF7, pull-up (idle high) */
    GPIOA->MODER  &= ~(0x3U << (UART_RX_PIN * 2));
    GPIOA->MODER  |=  (GPIO_MODER_AF << (UART_RX_PIN * 2));
    GPIOA->PUPDR  &= ~(0x3U << (UART_RX_PIN * 2));
    GPIOA->PUPDR  |=  (GPIO_PUPDR_PU << (UART_RX_PIN * 2));
    GPIOA->AFR[0] &= ~(0xFU << (UART_RX_PIN * 4));
    GPIOA->AFR[0] |=  (UART_AF << (UART_RX_PIN * 4));
#endif

    USART2->CR1 = 0;
    USART2->BRR = USART2_BRR_VAL;
    USART2->CR2 = 0;    /* 1 stop bit */
    USART2->CR3 = 0;    /* no flow control */
#ifdef ENABLE_UART_RX
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
#else
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE;
#endif
}

static void uart_putchar(char c) {
    while (!(USART2->SR & USART_SR_TXE));
    USART2->DR = (uint8_t)c;
}

static void uart_puts(const char *s) {
    while (*s)
        uart_putchar(*s++);
    while (!(USART2->SR & USART_SR_TC));    /* wait until last byte shifted out */
}

// Safe, zero-allocation conversion to a fixed buffer
static void uart_put_int(int32_t num) 
{
    char buffer[12]; // Enough for -2147483648 and null terminator
    int i = 0;
    bool isNegative = false;

    if (num == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        uart_puts(buffer);
        uart_putchar('\n');
        return;
    }

    if (num < 0) {
        isNegative = true;
        // Handle INT_MIN carefully to prevent overflow during inversion
        uint32_t unsignedNum = (num == -2147483648) ? 2147483648U : (uint32_t)(-num);
        while (unsignedNum != 0) {
            buffer[i++] = (unsignedNum % 10) + '0';
            unsignedNum /= 10;
        }
    } else {
        while (num != 0) {
            buffer[i++] = (num % 10) + '0';
            num /= 10;
        }
    }

    if (isNegative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    // Reverse the string in-place
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }

    uart_puts(buffer);
    uart_putchar('\n');
}

// Print a float with 3 decimal places, e.g. 12.340, without relying on snprintf.
static void uart_put_float(float value) {
    if (value > -0.0005f && value < 0.0005f) {
        value = 0.0f;
    }

    if (value < 0.0f) {
        uart_putchar('-');
        value = -value;
    }

    int whole = (int)value;
    int frac = (int)((value - (float)whole) * 1000.0f + 0.5f);
    if (frac >= 1000) {
        frac = 0;
        whole++;
    }

    char digits[12];
    int i = 0;
    int n = whole;

    if (n == 0) {
        digits[i++] = '0';
    } else {
        while (n > 0) {
            digits[i++] = (char)('0' + (n % 10));
            n /= 10;
        }
    }

    for (int j = i - 1; j >= 0; j--) {
        uart_putchar(digits[j]);
    }

    uart_putchar('.');
    uart_putchar((char)('0' + ((frac / 100) % 10)));
    uart_putchar((char)('0' + ((frac / 10) % 10)));
    uart_putchar((char)('0' + (frac % 10)));
    uart_putchar('\r');
    uart_putchar('\n');
}


#endif /* ENABLE_UART_TX */

/* ── UART RX command parser ──────────────────────────────────────────────
 * Compiled only when ENABLE_UART_RX is defined (implies ENABLE_UART_TX).
 *
 * Protocol (picocom-friendly):
 *   Send: "rate <period_ms>" + Enter
 *         period_ms = full blink cycle in ms (200–20000)
 *   Recv: "OK: period=2000ms\r\n"   or   "ERR: ...\r\n"
 *
 * Characters are echoed back; backspace (0x08 or 0x7F) is handled.
 * ─────────────────────────────────────────────────────────────────────── */
#ifdef ENABLE_UART_RX

static char    rx_buf[32];
static uint8_t rx_pos = 0;

// static void parse_command(void) {
//     const char *p = rx_buf;

//     /* skip leading whitespace */
//     while (*p == ' ') p++;

//     /* match "rate " */
//     if (p[0] == 'r' && p[1] == 'a' && p[2] == 't' && p[3] == 'e' && p[4] == ' ') {
//         p += 5;
//         while (*p == ' ') p++;

//         uint32_t val = 0;
//         if (*p < '0' || *p > '9') {
//             uart_puts("\r\nERR: 'rate' needs a number\r\n> ");
//             return;
//         }
//         while (*p >= '0' && *p <= '9')
//             val = val * 10U + (uint32_t)(*p++ - '0');

//         if (val < MIN_PERIOD_MS || val > MAX_PERIOD_MS) {
//             char err[48];
//             snprintf(err, sizeof(err), "\r\nERR: range %u-%u ms\r\n> ",
//                      (unsigned)MIN_PERIOD_MS, (unsigned)MAX_PERIOD_MS);
//             uart_puts(err);
//             return;
//         }

//         g_half_period_ms = val / 2U;

//         char ack[48];
//         snprintf(ack, sizeof(ack), "\r\nOK: period=%lu ms\r\n> ", (unsigned long)val);
//         uart_puts(ack);
//     } else if (rx_buf[0] == '\0') {
//         uart_puts("\r\n> ");   /* empty line — just re-print prompt */
//     } else {
//         uart_puts("\r\nERR: unknown command.  Try: rate 2000\r\n> ");
//     }
// }

static int rx_length()
{
    return rx_pos;
}

static void uart_rx_poll(void) {
    if (!(USART2->SR & USART_SR_RXNE))
        return;

    char c = (char)(USART2->DR & 0xFFU);

    // if (c == '\r' || c == '\n') {
    //     rx_buf[rx_pos] = '\0';
    //     rx_pos = 0;
    //     parse_command();
    // } else 
    if (c == '\b' || c == 127) {
        /* backspace / DEL */
        if (rx_pos > 0) {
            rx_pos--;
            uart_puts("\b \b");     /* erase character on terminal */
        }
    } else if (rx_pos < (uint8_t)(sizeof(rx_buf) - 1U)) {
        rx_buf[rx_pos++] = c;
        uart_putchar(c);            /* local echo */
    }
}

static void uart_rx_echo_prompt(void) {
    if (!(USART2->SR & USART_SR_RXNE))
        return;

    /* Read one byte from the UART and immediately echo it back out. */
    char c = (char)(USART2->DR & 0xFFU);
    uart_putchar(c);

    /* Optional: make terminal CRLF look nicer when Enter is pressed. */
    if (c == '\r')
        uart_putchar('\n');
}

#endif /* ENABLE_UART_RX */

/* ── EXAMPLE ───────────────────────────────────────────────────────────────── */

//uart_init();
//uart_puts("\r\n=== STM32F4DISCOVERY blink demo ===\r\n");
//uart_rx_poll();

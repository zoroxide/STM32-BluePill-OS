/**
 * @file    UART.c
 * @brief   UART HAL Implementation
 */

#include "UART.h"

#define RCC_BASE        0x40021000UL
#define GPIOA_BASE      0x40010800UL
#define GPIOB_BASE      0x40010C00UL
#define USART1_BASE     0x40013800UL
#define USART2_BASE     0x40004400UL
#define USART3_BASE     0x40004800UL

#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x1CUL))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x18UL))

#define USART_SR        0x00UL
#define USART_DR        0x04UL
#define USART_BRR       0x08UL
#define USART_CR1       0x0CUL

#define USART_SR_TXE    (1UL << 7)
#define USART_SR_RXNE   (1UL << 5)
#define USART_CR1_UE    (1UL << 13)
#define USART_CR1_TE    (1UL << 3)
#define USART_CR1_RE    (1UL << 2)
#define USART_CR1_RXNEIE (1UL << 5)

/*---------------------------------------------------------------------------*/
/* NVIC (needed to enable USART IRQs)                                        */
/*---------------------------------------------------------------------------*/
#define NVIC_ISER1      (*(volatile uint32_t *)0xE000E104UL)

/*---------------------------------------------------------------------------*/
/* RX Ring Buffer                                                            */
/*---------------------------------------------------------------------------*/

/** @brief Ring buffer structure for interrupt-driven RX */
typedef struct {
    uint8_t  buf[UART_RX_BUF_SIZE];
    volatile uint32_t head;  /**< Write index (ISR writes here) */
    volatile uint32_t tail;  /**< Read index (user reads here)  */
} UART_RxBuf_t;

static UART_RxBuf_t uart_rx_buf[3];

#define GPIO_AF_PP_50MHZ    0x0BUL
#define GPIO_INPUT_FLOAT    0x04UL

static uint32_t UART_GetBase(UART_Peripheral_t uart) {
    switch (uart) {
        case UART_1: return USART1_BASE;
        case UART_2: return USART2_BASE;
        case UART_3: return USART3_BASE;
        default:     return USART1_BASE;
    }
}

static inline uint32_t UART_ReadReg(uint32_t base, uint32_t off) {
    return *(volatile uint32_t *)(base + off);
}

static inline void UART_WriteReg(uint32_t base, uint32_t off, uint32_t val) {
    *(volatile uint32_t *)(base + off) = val;
}

static void UART_ConfigGPIO(uint32_t gpio, uint32_t pin, uint32_t cfg) {
    volatile uint32_t *cr;
    uint32_t shift;
    if (pin < 8) {
        cr = (volatile uint32_t *)(gpio + 0x00UL);
        shift = pin * 4;
    } else {
        cr = (volatile uint32_t *)(gpio + 0x04UL);
        shift = (pin - 8) * 4;
    }
    *cr &= ~(0xFUL << shift);
    *cr |= (cfg << shift);
}

static void UART_EnableClockAndPins(UART_Peripheral_t uart) {
    switch (uart) {
        case UART_1:
            RCC_APB2ENR |= (1UL << 14) | (1UL << 2);
            UART_ConfigGPIO(GPIOA_BASE, 9, GPIO_AF_PP_50MHZ);
            UART_ConfigGPIO(GPIOA_BASE, 10, GPIO_INPUT_FLOAT);
            break;
        case UART_2:
            RCC_APB1ENR |= (1UL << 17);
            RCC_APB2ENR |= (1UL << 2);
            UART_ConfigGPIO(GPIOA_BASE, 2, GPIO_AF_PP_50MHZ);
            UART_ConfigGPIO(GPIOA_BASE, 3, GPIO_INPUT_FLOAT);
            break;
        case UART_3:
            RCC_APB1ENR |= (1UL << 18);
            RCC_APB2ENR |= (1UL << 3);
            UART_ConfigGPIO(GPIOB_BASE, 10, GPIO_AF_PP_50MHZ);
            UART_ConfigGPIO(GPIOB_BASE, 11, GPIO_INPUT_FLOAT);
            break;
    }
}

void UART_Init(UART_Peripheral_t uart, uint32_t baud) {
    uint32_t base = UART_GetBase(uart);
    UART_EnableClockAndPins(uart);
    UART_WriteReg(base, USART_CR1, 0);
    uint32_t brr = (UART_PCLK_HZ + (baud / 2)) / baud;
    UART_WriteReg(base, USART_BRR, brr);
    UART_WriteReg(base, USART_CR1, USART_CR1_UE | USART_CR1_TE | USART_CR1_RE);
}

void UART_SendByte(UART_Peripheral_t uart, uint8_t data) {
    uint32_t base = UART_GetBase(uart);
    uint32_t timeout = UART_TIMEOUT;
    while (!(UART_ReadReg(base, USART_SR) & USART_SR_TXE)) {
        if (--timeout == 0) { return; }
    }
    UART_WriteReg(base, USART_DR, data);
}

int8_t UART_ReceiveByte(UART_Peripheral_t uart, uint8_t *data) {
    uint32_t base = UART_GetBase(uart);
    uint32_t timeout = UART_TIMEOUT;
    while (!(UART_ReadReg(base, USART_SR) & USART_SR_RXNE)) {
        if (--timeout == 0) { return -1; }
    }
    *data = (uint8_t)(UART_ReadReg(base, USART_DR) & 0xFFUL);
    return 0;
}

void UART_SendString(UART_Peripheral_t uart, const char *str) {
    while (*str) {
        UART_SendByte(uart, (uint8_t)*str);
        str++;
    }
}

void UART_SendBuffer(UART_Peripheral_t uart, const uint8_t *buf, uint32_t len) {
    uint32_t i;
    for (i = 0; i < len; i++) {
        UART_SendByte(uart, buf[i]);
    }
}

uint8_t UART_IsRxReady(UART_Peripheral_t uart) {
    uint32_t base = UART_GetBase(uart);
    return (UART_ReadReg(base, USART_SR) & USART_SR_RXNE) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
/* Interrupt-Driven UART RX                                                  */
/*---------------------------------------------------------------------------*/

void UART_EnableRxInterrupt(UART_Peripheral_t uart) {
    uint32_t base = UART_GetBase(uart);

    /* Clear ring buffer */
    uart_rx_buf[uart].head = 0;
    uart_rx_buf[uart].tail = 0;

    /* Enable RXNE interrupt in USART CR1 */
    *(volatile uint32_t *)(base + USART_CR1) |= USART_CR1_RXNEIE;

    /* Enable USART IRQ in NVIC (USART1=37, USART2=38, USART3=39) */
    /* IRQs 37-39 are in ISER1 (bits 5-7) */
    NVIC_ISER1 = (1UL << (5 + (uint8_t)uart));
}

uint32_t UART_RxAvailable(UART_Peripheral_t uart) {
    uint32_t h = uart_rx_buf[uart].head;
    uint32_t t = uart_rx_buf[uart].tail;
    return (h - t) & (UART_RX_BUF_SIZE - 1);
}

int8_t UART_RxRead(UART_Peripheral_t uart, uint8_t *data) {
    UART_RxBuf_t *rb = &uart_rx_buf[uart];
    if (rb->head == rb->tail) { return -1; }  /* Empty */
    *data = rb->buf[rb->tail & (UART_RX_BUF_SIZE - 1)];
    rb->tail++;
    return 0;
}

/**
 * @brief   Internal: ISR handler for a USART, stores byte in ring buffer
 * @param   idx     UART index (0=USART1, 1=USART2, 2=USART3)
 */
static void UART_IRQHandle(uint8_t idx) {
    static const uint32_t bases[3] = { USART1_BASE, USART2_BASE, USART3_BASE };
    uint32_t base = bases[idx];
    UART_RxBuf_t *rb = &uart_rx_buf[idx];

    if (*(volatile uint32_t *)(base + USART_SR) & USART_SR_RXNE) {
        uint8_t byte = (uint8_t)(*(volatile uint32_t *)(base + USART_DR) & 0xFF);
        uint32_t next = (rb->head + 1) & (UART_RX_BUF_SIZE - 1);
        if (next != (rb->tail & (UART_RX_BUF_SIZE - 1))) {
            rb->buf[rb->head & (UART_RX_BUF_SIZE - 1)] = byte;
            rb->head++;
        }
        /* If buffer full, byte is dropped (overflow protection) */
    }
}

/* ISR entry points (override weak aliases in startup.c) */
void USART1_IRQHandler(void) { UART_IRQHandle(0); }
void USART2_IRQHandler(void) { UART_IRQHandle(1); }
void USART3_IRQHandler(void) { UART_IRQHandle(2); }

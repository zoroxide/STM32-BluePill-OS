/**
 * @file    SPI.c
 * @brief   SPI HAL Implementation
 */

#include "SPI.h"

#define RCC_BASE        0x40021000UL
#define GPIOA_BASE      0x40010800UL
#define GPIOB_BASE      0x40010C00UL
#define SPI1_BASE       0x40013000UL
#define SPI2_BASE       0x40003800UL

#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x1CUL))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x18UL))

#define SPI_CR1_OFF     0x00UL
#define SPI_SR_OFF      0x08UL
#define SPI_DR_OFF      0x0CUL

#define SPI_CR1_CPHA    (1UL << 0)
#define SPI_CR1_CPOL    (1UL << 1)
#define SPI_CR1_MSTR    (1UL << 2)
#define SPI_CR1_SPE     (1UL << 6)
#define SPI_CR1_SSI     (1UL << 8)
#define SPI_CR1_SSM     (1UL << 9)

#define SPI_SR_RXNE     (1UL << 0)
#define SPI_SR_TXE      (1UL << 1)

#define GPIO_AF_PP_50MHZ    0x0BUL
#define GPIO_INPUT_FLOAT    0x04UL

static uint32_t SPI_GetBase(SPI_Peripheral_t spi) {
    return (spi == SPI_1) ? SPI1_BASE : SPI2_BASE;
}

static inline uint32_t SPI_ReadReg(uint32_t base, uint32_t off) {
    return *(volatile uint32_t *)(base + off);
}

static inline void SPI_WriteReg(uint32_t base, uint32_t off, uint32_t val) {
    *(volatile uint32_t *)(base + off) = val;
}

static void SPI_ConfigGPIO(uint32_t gpio, uint32_t pin, uint32_t cfg) {
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

void SPI_Init(SPI_Peripheral_t spi, SPI_Prescaler_t prescaler, SPI_Mode_t mode) {
    uint32_t base = SPI_GetBase(spi);
    uint32_t cr1_val;

    if (spi == SPI_1) {
        RCC_APB2ENR |= (1UL << 12) | (1UL << 2);
        SPI_ConfigGPIO(GPIOA_BASE, 5, GPIO_AF_PP_50MHZ);
        SPI_ConfigGPIO(GPIOA_BASE, 6, GPIO_INPUT_FLOAT);
        SPI_ConfigGPIO(GPIOA_BASE, 7, GPIO_AF_PP_50MHZ);
    } else {
        RCC_APB1ENR |= (1UL << 14);
        RCC_APB2ENR |= (1UL << 3);
        SPI_ConfigGPIO(GPIOB_BASE, 13, GPIO_AF_PP_50MHZ);
        SPI_ConfigGPIO(GPIOB_BASE, 14, GPIO_INPUT_FLOAT);
        SPI_ConfigGPIO(GPIOB_BASE, 15, GPIO_AF_PP_50MHZ);
    }

    SPI_WriteReg(base, SPI_CR1_OFF, 0);

    cr1_val = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;
    cr1_val |= ((uint32_t)prescaler << 3);
    if (mode & 0x01) { cr1_val |= SPI_CR1_CPHA; }
    if (mode & 0x02) { cr1_val |= SPI_CR1_CPOL; }
    cr1_val |= SPI_CR1_SPE;

    SPI_WriteReg(base, SPI_CR1_OFF, cr1_val);
}

uint8_t SPI_TransmitReceive8(SPI_Peripheral_t spi, uint8_t tx_data) {
    uint32_t base = SPI_GetBase(spi);
    uint32_t timeout;

    timeout = SPI_TIMEOUT;
    while (!(SPI_ReadReg(base, SPI_SR_OFF) & SPI_SR_TXE)) {
        if (--timeout == 0) { return 0xFF; }
    }
    *(volatile uint8_t *)(base + SPI_DR_OFF) = tx_data;

    timeout = SPI_TIMEOUT;
    while (!(SPI_ReadReg(base, SPI_SR_OFF) & SPI_SR_RXNE)) {
        if (--timeout == 0) { return 0xFF; }
    }
    return *(volatile uint8_t *)(base + SPI_DR_OFF);
}

void SPI_Transmit(SPI_Peripheral_t spi, const uint8_t *data, uint32_t len) {
    uint32_t i;
    for (i = 0; i < len; i++) { (void)SPI_TransmitReceive8(spi, data[i]); }
}

void SPI_Receive(SPI_Peripheral_t spi, uint8_t *data, uint32_t len) {
    uint32_t i;
    for (i = 0; i < len; i++) { data[i] = SPI_TransmitReceive8(spi, 0xFF); }
}

void SPI_TransferBuffer(SPI_Peripheral_t spi, const uint8_t *tx_buf,
                        uint8_t *rx_buf, uint32_t len) {
    uint32_t i;
    for (i = 0; i < len; i++) { rx_buf[i] = SPI_TransmitReceive8(spi, tx_buf[i]); }
}

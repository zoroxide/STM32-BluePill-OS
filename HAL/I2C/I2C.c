/**
 * @file    I2C.c
 * @brief   I2C HAL Implementation
 */

#include "I2C.h"

#define RCC_BASE        0x40021000UL
#define GPIOB_BASE      0x40010C00UL
#define I2C1_BASE       0x40005400UL
#define I2C2_BASE       0x40005800UL

#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x1CUL))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x18UL))

#define I2C_CR1_OFF     0x00UL
#define I2C_CR2_OFF     0x04UL
#define I2C_DR_OFF      0x10UL
#define I2C_SR1_OFF     0x14UL
#define I2C_SR2_OFF     0x18UL
#define I2C_CCR_OFF     0x1CUL
#define I2C_TRISE_OFF   0x20UL

#define I2C_CR1_PE      (1UL << 0)
#define I2C_CR1_START   (1UL << 8)
#define I2C_CR1_STOP    (1UL << 9)
#define I2C_CR1_ACK     (1UL << 10)
#define I2C_CR1_SWRST   (1UL << 15)

#define I2C_SR1_SB      (1UL << 0)
#define I2C_SR1_ADDR    (1UL << 1)
#define I2C_SR1_BTF     (1UL << 2)
#define I2C_SR1_RXNE    (1UL << 6)
#define I2C_SR1_TXE     (1UL << 7)

#define I2C_CCR_FS      (1UL << 15)
#define GPIO_AF_OD_50MHZ 0x0FUL

static uint32_t I2C_GetBase(I2C_Peripheral_t i2c) {
    return (i2c == I2C_1) ? I2C1_BASE : I2C2_BASE;
}

static inline uint32_t I2C_Rd(uint32_t base, uint32_t off) {
    return *(volatile uint32_t *)(base + off);
}

static inline void I2C_Wr(uint32_t base, uint32_t off, uint32_t val) {
    *(volatile uint32_t *)(base + off) = val;
}

static void I2C_ConfigGPIO(uint32_t gpio, uint32_t pin, uint32_t cfg) {
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

static int8_t I2C_WaitFlag(uint32_t base, uint32_t flag) {
    uint32_t timeout = I2C_TIMEOUT;
    while (!(I2C_Rd(base, I2C_SR1_OFF) & flag)) {
        if (--timeout == 0) { return -1; }
    }
    return 0;
}

static int8_t I2C_StartAndAddress(uint32_t base, uint8_t addr, uint8_t rw) {
    *(volatile uint32_t *)(base + I2C_CR1_OFF) |= I2C_CR1_START;
    if (I2C_WaitFlag(base, I2C_SR1_SB) != 0) { return -1; }
    I2C_Wr(base, I2C_DR_OFF, (uint32_t)((addr << 1) | rw));
    if (I2C_WaitFlag(base, I2C_SR1_ADDR) != 0) { return -1; }
    (void)I2C_Rd(base, I2C_SR1_OFF);
    (void)I2C_Rd(base, I2C_SR2_OFF);
    return 0;
}

void I2C_Init(I2C_Peripheral_t i2c, I2C_Speed_t speed) {
    uint32_t base = I2C_GetBase(i2c);
    uint32_t pclk_mhz = I2C_PCLK_HZ / 1000000UL;
    uint32_t ccr_val;

    RCC_APB2ENR |= (1UL << 3);

    if (i2c == I2C_1) {
        RCC_APB1ENR |= (1UL << 21);
        I2C_ConfigGPIO(GPIOB_BASE, 6, GPIO_AF_OD_50MHZ);
        I2C_ConfigGPIO(GPIOB_BASE, 7, GPIO_AF_OD_50MHZ);
    } else {
        RCC_APB1ENR |= (1UL << 22);
        I2C_ConfigGPIO(GPIOB_BASE, 10, GPIO_AF_OD_50MHZ);
        I2C_ConfigGPIO(GPIOB_BASE, 11, GPIO_AF_OD_50MHZ);
    }

    I2C_Wr(base, I2C_CR1_OFF, I2C_CR1_SWRST);
    I2C_Wr(base, I2C_CR1_OFF, 0);
    I2C_Wr(base, I2C_CR2_OFF, pclk_mhz);

    if ((uint32_t)speed == 400000UL) {
        ccr_val = I2C_PCLK_HZ / (3 * 400000UL);
        if (ccr_val < 1) { ccr_val = 1; }
        I2C_Wr(base, I2C_CCR_OFF, I2C_CCR_FS | ccr_val);
        I2C_Wr(base, I2C_TRISE_OFF, (pclk_mhz * 300 / 1000) + 1);
    } else {
        ccr_val = I2C_PCLK_HZ / (2 * 100000UL);
        if (ccr_val < 4) { ccr_val = 4; }
        I2C_Wr(base, I2C_CCR_OFF, ccr_val);
        I2C_Wr(base, I2C_TRISE_OFF, pclk_mhz + 1);
    }

    I2C_Wr(base, I2C_CR1_OFF, I2C_CR1_PE | I2C_CR1_ACK);
}

int8_t I2C_Write(I2C_Peripheral_t i2c, uint8_t addr,
                 const uint8_t *data, uint32_t len) {
    uint32_t base = I2C_GetBase(i2c);
    uint32_t i;

    if (I2C_StartAndAddress(base, addr, 0) != 0) {
        *(volatile uint32_t *)(base + I2C_CR1_OFF) |= I2C_CR1_STOP;
        return -1;
    }
    for (i = 0; i < len; i++) {
        if (I2C_WaitFlag(base, I2C_SR1_TXE) != 0) {
            *(volatile uint32_t *)(base + I2C_CR1_OFF) |= I2C_CR1_STOP;
            return -1;
        }
        I2C_Wr(base, I2C_DR_OFF, data[i]);
    }
    if (I2C_WaitFlag(base, I2C_SR1_BTF) != 0) {
        *(volatile uint32_t *)(base + I2C_CR1_OFF) |= I2C_CR1_STOP;
        return -1;
    }
    *(volatile uint32_t *)(base + I2C_CR1_OFF) |= I2C_CR1_STOP;
    return 0;
}

int8_t I2C_Read(I2C_Peripheral_t i2c, uint8_t addr,
                uint8_t *data, uint32_t len) {
    uint32_t base = I2C_GetBase(i2c);
    uint32_t i;

    if (len == 0) { return -1; }
    *(volatile uint32_t *)(base + I2C_CR1_OFF) |= I2C_CR1_ACK;

    if (I2C_StartAndAddress(base, addr, 1) != 0) {
        *(volatile uint32_t *)(base + I2C_CR1_OFF) |= I2C_CR1_STOP;
        return -1;
    }
    for (i = 0; i < len; i++) {
        if (i == len - 1) {
            *(volatile uint32_t *)(base + I2C_CR1_OFF) &= ~I2C_CR1_ACK;
            *(volatile uint32_t *)(base + I2C_CR1_OFF) |= I2C_CR1_STOP;
        }
        if (I2C_WaitFlag(base, I2C_SR1_RXNE) != 0) {
            *(volatile uint32_t *)(base + I2C_CR1_OFF) |= I2C_CR1_STOP;
            return -1;
        }
        data[i] = (uint8_t)(I2C_Rd(base, I2C_DR_OFF) & 0xFFUL);
    }
    return 0;
}

int8_t I2C_WriteReg(I2C_Peripheral_t i2c, uint8_t addr,
                    uint8_t reg, uint8_t value) {
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = value;
    return I2C_Write(i2c, addr, buf, 2);
}

int8_t I2C_ReadReg(I2C_Peripheral_t i2c, uint8_t addr,
                   uint8_t reg, uint8_t *value) {
    uint32_t base = I2C_GetBase(i2c);

    if (I2C_StartAndAddress(base, addr, 0) != 0) {
        *(volatile uint32_t *)(base + I2C_CR1_OFF) |= I2C_CR1_STOP;
        return -1;
    }
    if (I2C_WaitFlag(base, I2C_SR1_TXE) != 0) {
        *(volatile uint32_t *)(base + I2C_CR1_OFF) |= I2C_CR1_STOP;
        return -1;
    }
    I2C_Wr(base, I2C_DR_OFF, reg);
    if (I2C_WaitFlag(base, I2C_SR1_BTF) != 0) {
        *(volatile uint32_t *)(base + I2C_CR1_OFF) |= I2C_CR1_STOP;
        return -1;
    }
    return I2C_Read(i2c, addr, value, 1);
}

/**
 * @file    DAC.c
 * @brief   DAC HAL Implementation
 *
 * @warning The STM32F103C8T6 (Blue Pill) does NOT have a DAC peripheral.
 *          This driver is for larger STM32F103 variants only.
 */

#include "DAC.h"

#define RCC_BASE        0x40021000UL
#define GPIOA_BASE      0x40010800UL
#define DAC_BASE_ADDR   0x40007400UL

#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x1CUL))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x18UL))

#define DAC_CR_OFF      0x00UL
#define DAC_DHR12R1_OFF 0x08UL
#define DAC_DHR12R2_OFF 0x14UL

#define DAC_CR_EN1      (1UL << 0)
#define DAC_CR_EN2      (1UL << 16)

static inline void DAC_Wr(uint32_t offset, uint32_t value) {
    *(volatile uint32_t *)(DAC_BASE_ADDR + offset) = value;
}

static void DAC_ConfigAnalogPin(uint32_t pin) {
    volatile uint32_t *crl = (volatile uint32_t *)(GPIOA_BASE + 0x00UL);
    uint32_t shift = pin * 4;
    *crl &= ~(0xFUL << shift);
}

void DAC_Init(DAC_Channel_t channel) {
    RCC_APB2ENR |= (1UL << 2);
    RCC_APB1ENR |= (1UL << 29);

    if (channel == DAC_CHANNEL_1) {
        DAC_ConfigAnalogPin(4);
        *(volatile uint32_t *)(DAC_BASE_ADDR + DAC_CR_OFF) |= DAC_CR_EN1;
        DAC_Wr(DAC_DHR12R1_OFF, 0);
    } else {
        DAC_ConfigAnalogPin(5);
        *(volatile uint32_t *)(DAC_BASE_ADDR + DAC_CR_OFF) |= DAC_CR_EN2;
        DAC_Wr(DAC_DHR12R2_OFF, 0);
    }
}

void DAC_Write(DAC_Channel_t channel, uint16_t value) {
    if (value > DAC_MAX_VALUE) { value = (uint16_t)DAC_MAX_VALUE; }
    if (channel == DAC_CHANNEL_1) {
        DAC_Wr(DAC_DHR12R1_OFF, value);
    } else {
        DAC_Wr(DAC_DHR12R2_OFF, value);
    }
}

void DAC_WriteMillivolts(DAC_Channel_t channel, uint32_t mv) {
    if (mv > DAC_VREF_MV) { mv = DAC_VREF_MV; }
    uint16_t value = (uint16_t)((mv * DAC_MAX_VALUE) / DAC_VREF_MV);
    DAC_Write(channel, value);
}

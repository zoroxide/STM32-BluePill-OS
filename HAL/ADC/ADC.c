/**
 * @file    ADC.c
 * @brief   ADC HAL Implementation
 */

#include "ADC.h"

#define RCC_BASE        0x40021000UL
#define GPIOA_BASE      0x40010800UL
#define GPIOB_BASE      0x40010C00UL
#define ADC1_BASE_ADDR  0x40012400UL
#define ADC2_BASE_ADDR  0x40012800UL

#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x18UL))

#define ADC_SR_OFF      0x00UL
#define ADC_CR2_OFF     0x08UL
#define ADC_SMPR1_OFF   0x0CUL
#define ADC_SMPR2_OFF   0x10UL
#define ADC_SQR1_OFF    0x2CUL
#define ADC_SQR3_OFF    0x34UL
#define ADC_DR_OFF      0x4CUL

#define ADC_SR_EOC      (1UL << 1)
#define ADC_CR2_ADON    (1UL << 0)
#define ADC_CR2_CAL     (1UL << 2)
#define ADC_CR2_RSTCAL  (1UL << 3)
#define ADC_CR2_EXTTRIG (1UL << 20)
#define ADC_CR2_SWSTART (1UL << 22)
#define ADC_CR2_TSVREFE (1UL << 23)
#define ADC_CR2_EXTSEL_SW (7UL << 17)

static uint32_t ADC_GetBase(ADC_Peripheral_t adc) {
    return (adc == ADC_1) ? ADC1_BASE_ADDR : ADC2_BASE_ADDR;
}

static inline uint32_t ADC_Rd(uint32_t base, uint32_t off) {
    return *(volatile uint32_t *)(base + off);
}

static inline void ADC_Wr(uint32_t base, uint32_t off, uint32_t val) {
    *(volatile uint32_t *)(base + off) = val;
}

static void ADC_Delay(volatile uint32_t count) {
    while (count--) {}
}

static void ADC_ConfigAnalogPin(uint32_t gpio, uint32_t pin) {
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
}

void ADC_Init(ADC_Peripheral_t adc) {
    uint32_t base = ADC_GetBase(adc);
    uint32_t timeout;

    if (adc == ADC_1) {
        RCC_APB2ENR |= (1UL << 9);
    } else {
        RCC_APB2ENR |= (1UL << 10);
    }
    RCC_APB2ENR |= (1UL << 2) | (1UL << 3);

    ADC_Wr(base, ADC_CR2_OFF,
           ADC_CR2_ADON | ADC_CR2_EXTTRIG | ADC_CR2_EXTSEL_SW);
    ADC_Delay(100);

    *(volatile uint32_t *)(base + ADC_CR2_OFF) |= ADC_CR2_RSTCAL;
    timeout = ADC_TIMEOUT;
    while ((ADC_Rd(base, ADC_CR2_OFF) & ADC_CR2_RSTCAL) && --timeout) {}

    *(volatile uint32_t *)(base + ADC_CR2_OFF) |= ADC_CR2_CAL;
    timeout = ADC_TIMEOUT;
    while ((ADC_Rd(base, ADC_CR2_OFF) & ADC_CR2_CAL) && --timeout) {}

    /* 239.5 cycles sample time for all channels */
    ADC_Wr(base, ADC_SMPR2_OFF, 0x3FFFFFFFUL);
    ADC_Wr(base, ADC_SMPR1_OFF, 0x00FFFFFFUL);
    ADC_Wr(base, ADC_SQR1_OFF, 0);
}

uint16_t ADC_Read(ADC_Peripheral_t adc, ADC_Channel_t channel) {
    uint32_t base = ADC_GetBase(adc);
    uint32_t timeout;

    if ((uint32_t)channel <= 7) {
        ADC_ConfigAnalogPin(GPIOA_BASE, (uint32_t)channel);
    } else if ((uint32_t)channel <= 9) {
        ADC_ConfigAnalogPin(GPIOB_BASE, (uint32_t)channel - 8);
    }

    if (channel == ADC_CH_TEMP || channel == ADC_CH_VREF) {
        *(volatile uint32_t *)(base + ADC_CR2_OFF) |= ADC_CR2_TSVREFE;
        ADC_Delay(1000);
    }

    ADC_Wr(base, ADC_SQR3_OFF, (uint32_t)channel);
    *(volatile uint32_t *)(base + ADC_SR_OFF) &= ~ADC_SR_EOC;
    *(volatile uint32_t *)(base + ADC_CR2_OFF) |= ADC_CR2_SWSTART;

    timeout = ADC_TIMEOUT;
    while (!(ADC_Rd(base, ADC_SR_OFF) & ADC_SR_EOC)) {
        if (--timeout == 0) { return 0; }
    }
    return (uint16_t)(ADC_Rd(base, ADC_DR_OFF) & 0x0FFFUL);
}

uint32_t ADC_ReadMillivolts(ADC_Peripheral_t adc, ADC_Channel_t channel) {
    uint16_t raw = ADC_Read(adc, channel);
    return ((uint32_t)raw * ADC_VREF_MV) / ADC_MAX_VALUE;
}

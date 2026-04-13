/**
 * @file IO.c
 * @brief GPIO Hardware Abstraction Layer Implementation
 *
 * Implementation of GPIO control functions for STM32F103 microcontroller.
 * Provides safe, register-level abstractions for GPIO operations.
 */

#include "IO.h"

/* STM32F103 Register Base Addresses */
#define RCC_BASE        0x40021000
#define GPIOA_BASE      0x40010800
#define GPIOB_BASE      0x40010C00
#define GPIOC_BASE      0x40011000

/* RCC APB2 Peripheral Clock Enable Register */
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x18))

/* GPIO Port Control Register Offsets */
#define GPIO_CRL        0x00  /* Port A/B/C Lower Register (pins 0-7) */
#define GPIO_CRH        0x04  /* Port A/B/C Higher Register (pins 8-15) */
#define GPIO_IDR        0x08  /* Input Data Register  */
#define GPIO_ODR        0x0C  /* Output Data Register */

/**
 * @brief Get GPIO port base address
 *
 * @param port GPIO port enumeration
 * @return Base address of the GPIO port
 */
static volatile uint32_t* GPIO_GetPortBase(GPIO_Port_t port) {
    switch (port) {
        case GPIO_PORT_A:
            return (volatile uint32_t*)GPIOA_BASE;
        case GPIO_PORT_B:
            return (volatile uint32_t*)GPIOB_BASE;
        case GPIO_PORT_C:
            return (volatile uint32_t*)GPIOC_BASE;
        default:
            return (volatile uint32_t*)GPIOA_BASE;
    }
}

/**
 * @brief Enable clock for GPIO port
 *
 * @param port GPIO port enumeration
 */
static void GPIO_EnableClock(GPIO_Port_t port) {
    /* Set appropriate IOPAEN, IOPBEN, or IOPCEN bit in RCC_APB2ENR */
    RCC_APB2ENR |= (1 << (2 + port));
}

/**
 * @brief Configure pin as output in control register
 *
 * Sets the pin configuration bits to output mode (binary 10) at 50 MHz.
 * For pins 0-7: use CRL, for pins 8-15: use CRH.
 *
 * @param port GPIO port
 * @param pin Pin number (0-15)
 */
static void GPIO_ConfigurePin(GPIO_Port_t port, GPIO_Pin_t pin) {
    volatile uint32_t *port_base = GPIO_GetPortBase(port);
    volatile uint32_t *control_reg;
    uint32_t pin_offset;
    
    /* Select CRL or CRH based on pin number */
    if (pin < 8) {
        control_reg = (volatile uint32_t *)(((uint32_t)port_base) + GPIO_CRL);
        pin_offset = pin * 4;
    } else {
        control_reg = (volatile uint32_t *)(((uint32_t)port_base) + GPIO_CRH);
        pin_offset = (pin - 8) * 4;
    }
    
    /* Clear the 4 bits for this pin */
    *control_reg &= ~(0xF << pin_offset);
    
    /* Set to output mode (binary 10) at 50 MHz (binary 10) = 0x2 */
    *control_reg |= (0x2 << pin_offset);
}

void GPIO_Init(GPIO_Port_t port, GPIO_Pin_t pin) {
    /* Enable clock for the port */
    GPIO_EnableClock(port);
    
    /* Configure pin as output */
    GPIO_ConfigurePin(port, pin);
    
    /* Set pin LOW initially */
    GPIO_ClrPin(port, pin);
}

void GPIO_SetPin(GPIO_Port_t port, GPIO_Pin_t pin) {
    volatile uint32_t *port_base = GPIO_GetPortBase(port);
    volatile uint32_t *odr = (volatile uint32_t *)(((uint32_t)port_base) + GPIO_ODR);
    
    /* Set the bit for this pin in the Output Data Register */
    *odr |= (1 << pin);
}

void GPIO_ClrPin(GPIO_Port_t port, GPIO_Pin_t pin) {
    volatile uint32_t *port_base = GPIO_GetPortBase(port);
    volatile uint32_t *odr = (volatile uint32_t *)(((uint32_t)port_base) + GPIO_ODR);
    
    /* Clear the bit for this pin in the Output Data Register */
    *odr &= ~(1 << pin);
}

void GPIO_TogglePin(GPIO_Port_t port, GPIO_Pin_t pin) {
    volatile uint32_t *port_base = GPIO_GetPortBase(port);
    volatile uint32_t *odr = (volatile uint32_t *)(((uint32_t)port_base) + GPIO_ODR);

    /* Toggle the bit for this pin in the Output Data Register */
    *odr ^= (1 << pin);
}

void GPIO_InitInput(GPIO_Port_t port, GPIO_Pin_t pin) {
    volatile uint32_t *port_base;
    volatile uint32_t *control_reg;
    uint32_t pin_offset;

    /* Enable clock for the port */
    GPIO_EnableClock(port);

    port_base = GPIO_GetPortBase(port);

    /* Select CRL or CRH based on pin number */
    if (pin < 8) {
        control_reg = (volatile uint32_t *)(((uint32_t)port_base) + GPIO_CRL);
        pin_offset = pin * 4;
    } else {
        control_reg = (volatile uint32_t *)(((uint32_t)port_base) + GPIO_CRH);
        pin_offset = (pin - 8) * 4;
    }

    /* Clear the 4 bits for this pin */
    *control_reg &= ~(0xF << pin_offset);

    /* Set to input with pull-up/pull-down: CNF=10 MODE=00 = 0x8 */
    *control_reg |= (0x8 << pin_offset);

    /* Set ODR bit to select pull-UP (not pull-down) */
    *(volatile uint32_t *)(((uint32_t)port_base) + GPIO_ODR) |= (1 << pin);
}

uint8_t GPIO_ReadPin(GPIO_Port_t port, GPIO_Pin_t pin) {
    volatile uint32_t *port_base = GPIO_GetPortBase(port);
    volatile uint32_t *idr = (volatile uint32_t *)(((uint32_t)port_base) + GPIO_IDR);

    return (*idr & (1 << pin)) ? 1 : 0;
}

/**
 * @file IO.h
 * @brief GPIO (General Purpose Input/Output) Hardware Abstraction Layer
 *
 * This module provides a simple and safe interface for controlling GPIO pins
 * on the STM32F103 microcontroller. It abstracts away low-level register
 * manipulation and provides easy-to-use functions for pin control.
 *
 * @author STM32 Blue Pill Project
 * @version 1.0
 */

#ifndef HAL_IO_H
#define HAL_IO_H

#include <stdint.h>

/**
 * @enum GPIO_Port_t
 * @brief Enumeration of GPIO ports on STM32F103
 */
typedef enum {
    GPIO_PORT_A = 0,  /**< GPIO Port A */
    GPIO_PORT_B = 1,  /**< GPIO Port B */
    GPIO_PORT_C = 2,  /**< GPIO Port C */
} GPIO_Port_t;

/**
 * @enum GPIO_Pin_t
 * @brief Enumeration of GPIO pins (0-15)
 */
typedef enum {
    GPIO_PIN_0  = 0,
    GPIO_PIN_1  = 1,
    GPIO_PIN_2  = 2,
    GPIO_PIN_3  = 3,
    GPIO_PIN_4  = 4,
    GPIO_PIN_5  = 5,
    GPIO_PIN_6  = 6,
    GPIO_PIN_7  = 7,
    GPIO_PIN_8  = 8,
    GPIO_PIN_9  = 9,
    GPIO_PIN_10 = 10,
    GPIO_PIN_11 = 11,
    GPIO_PIN_12 = 12,
    GPIO_PIN_13 = 13,
    GPIO_PIN_14 = 14,
    GPIO_PIN_15 = 15,
} GPIO_Pin_t;

/**
 * @brief Initialize a GPIO pin as digital output
 *
 * Configures the specified pin on the given port as a push-pull output
 * with a 50 MHz slew rate. The pin will be set LOW after initialization.
 *
 * @param port GPIO port (GPIO_PORT_A, GPIO_PORT_B, GPIO_PORT_C)
 * @param pin GPIO pin number (0-15)
 *
 * @note Multiple pins can be initialized by calling this function multiple times.
 * @return None
 */
void GPIO_Init(GPIO_Port_t port, GPIO_Pin_t pin);

/**
 * @brief Set a GPIO pin to HIGH (logic 1)
 *
 * Sets the output of the specified pin to logic HIGH (typically 3.3V).
 *
 * @param port GPIO port (GPIO_PORT_A, GPIO_PORT_B, GPIO_PORT_C)
 * @param pin GPIO pin number (0-15)
 *
 * @return None
 */
void GPIO_SetPin(GPIO_Port_t port, GPIO_Pin_t pin);

/**
 * @brief Set a GPIO pin to LOW (logic 0)
 *
 * Sets the output of the specified pin to logic LOW (0V).
 *
 * @param port GPIO port (GPIO_PORT_A, GPIO_PORT_B, GPIO_PORT_C)
 * @param pin GPIO pin number (0-15)
 *
 * @return None
 */
void GPIO_ClrPin(GPIO_Port_t port, GPIO_Pin_t pin);

/**
 * @brief Toggle a GPIO pin state
 *
 * Toggles the output state of the specified pin. If the pin is currently
 * HIGH, it will be set LOW, and vice versa.
 *
 * @param port GPIO port (GPIO_PORT_A, GPIO_PORT_B, GPIO_PORT_C)
 * @param pin GPIO pin number (0-15)
 *
 * @return None
 */
void GPIO_TogglePin(GPIO_Port_t port, GPIO_Pin_t pin);

/**
 * @brief Initialize a GPIO pin as digital input with pull-up
 *
 * Configures the specified pin as an input with an internal pull-up resistor.
 *
 * @param port GPIO port (GPIO_PORT_A, GPIO_PORT_B, GPIO_PORT_C)
 * @param pin GPIO pin number (0-15)
 *
 * @return None
 */
void GPIO_InitInput(GPIO_Port_t port, GPIO_Pin_t pin);

/**
 * @brief Read the logic state of a GPIO pin
 *
 * Reads the current input level of the specified pin.
 *
 * @param port GPIO port (GPIO_PORT_A, GPIO_PORT_B, GPIO_PORT_C)
 * @param pin GPIO pin number (0-15)
 *
 * @return 1 if pin is HIGH, 0 if pin is LOW
 */
uint8_t GPIO_ReadPin(GPIO_Port_t port, GPIO_Pin_t pin);

#endif /* HAL_IO_H */

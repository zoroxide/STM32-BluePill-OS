/**
 * @file    DAC.h
 * @brief   DAC (Digital-to-Analog Converter) HAL
 *
 * Provides a simple interface for digital-to-analog conversion on
 * STM32F103 variants that include a DAC peripheral.
 *
 * Pin mapping:
 *   - DAC Channel 1: PA4
 *   - DAC Channel 2: PA5
 *
 * @warning The STM32F103C8T6 (Blue Pill) does NOT have a DAC peripheral.
 *          This driver is for larger STM32F103 variants (RCT6, VCT6, ZET6)
 *          that include the DAC. Using this on a C8T6 will have no effect.
 *
 * @note    DAC output resolution is 12-bit (0-4095). Output range is 0V to VREF (3.3V).
 *
 * @version 1.0
 */

#ifndef HAL_DAC_H
#define HAL_DAC_H

#include <stdint.h>

/** @brief DAC reference voltage in millivolts */
#define DAC_VREF_MV     3300UL

/** @brief DAC maximum digital value (12-bit) */
#define DAC_MAX_VALUE   4095UL

/**
 * @enum DAC_Channel_t
 * @brief Available DAC output channels
 */
typedef enum {
    DAC_CHANNEL_1 = 0,  /**< DAC Channel 1 - output on PA4 */
    DAC_CHANNEL_2 = 1,  /**< DAC Channel 2 - output on PA5 */
} DAC_Channel_t;

/**
 * @brief   Initialize a DAC channel
 *
 * Enables the DAC peripheral clock, configures the output pin as
 * analog, and enables the specified channel.
 *
 * @param   channel DAC channel to initialize
 */
void DAC_Init(DAC_Channel_t channel);

/**
 * @brief   Write a raw 12-bit value to the DAC
 *
 * Sets the DAC output to the specified digital value.
 * Output voltage = (value / 4095) * VREF
 *
 * @param   channel DAC channel
 * @param   value   12-bit value (0-4095), clamped if out of range
 */
void DAC_Write(DAC_Channel_t channel, uint16_t value);

/**
 * @brief   Write a voltage in millivolts to the DAC
 *
 * Converts the millivolt value to a 12-bit digital value and sets
 * the DAC output accordingly.
 *
 * @param   channel DAC channel
 * @param   mv      Desired output voltage in millivolts (0-3300)
 */
void DAC_WriteMillivolts(DAC_Channel_t channel, uint32_t mv);

#endif /* HAL_DAC_H */

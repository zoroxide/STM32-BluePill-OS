/**
 * @file    ADC.h
 * @brief   ADC (Analog-to-Digital Converter) HAL
 *
 * Provides a simple interface for analog-to-digital conversion on the
 * STM32F103. Supports ADC1 and ADC2 with single-conversion mode.
 *
 * Channel-to-pin mapping:
 *   - CH0  = PA0     - CH4  = PA4     - CH8  = PB0
 *   - CH1  = PA1     - CH5  = PA5     - CH9  = PB1
 *   - CH2  = PA2     - CH6  = PA6     - CH16 = Temperature sensor
 *   - CH3  = PA3     - CH7  = PA7     - CH17 = Vrefint (1.2V)
 *
 * @note    ADC resolution is 12-bit (0-4095). Reference voltage is 3.3V.
 * @note    Temperature sensor and Vrefint are only available on ADC1.
 *
 * @version 1.0
 */

#ifndef HAL_ADC_H
#define HAL_ADC_H

#include <stdint.h>

/** @brief ADC reference voltage in millivolts */
#define ADC_VREF_MV     3300UL

/** @brief ADC maximum digital value (12-bit) */
#define ADC_MAX_VALUE   4095UL

/** @brief Default timeout for conversion (loop iterations) */
#define ADC_TIMEOUT     100000UL

/**
 * @enum ADC_Peripheral_t
 * @brief Available ADC peripherals
 */
typedef enum {
    ADC_1 = 0,  /**< ADC1 (APB2) - supports temp sensor & Vrefint */
    ADC_2 = 1,  /**< ADC2 (APB2)                                  */
} ADC_Peripheral_t;

/**
 * @enum ADC_Channel_t
 * @brief ADC input channels
 */
typedef enum {
    ADC_CH0  = 0,   /**< PA0 */
    ADC_CH1  = 1,   /**< PA1 */
    ADC_CH2  = 2,   /**< PA2 */
    ADC_CH3  = 3,   /**< PA3 */
    ADC_CH4  = 4,   /**< PA4 */
    ADC_CH5  = 5,   /**< PA5 */
    ADC_CH6  = 6,   /**< PA6 */
    ADC_CH7  = 7,   /**< PA7 */
    ADC_CH8  = 8,   /**< PB0 */
    ADC_CH9  = 9,   /**< PB1 */
    ADC_CH_TEMP = 16,  /**< Internal temperature sensor (ADC1 only) */
    ADC_CH_VREF = 17,  /**< Internal Vrefint 1.2V (ADC1 only)      */
} ADC_Channel_t;

/**
 * @brief   Initialize ADC peripheral
 *
 * Enables the peripheral clock, performs calibration, and configures
 * for single-conversion software-triggered mode.
 *
 * @param   adc     ADC peripheral to initialize
 */
void ADC_Init(ADC_Peripheral_t adc);

/**
 * @brief   Read a single ADC conversion (raw 12-bit value)
 *
 * Selects the specified channel, starts a conversion, waits for
 * completion, and returns the raw digital value.
 *
 * @param   adc     ADC peripheral
 * @param   channel ADC input channel
 * @return  12-bit conversion result (0-4095), or 0 on timeout
 */
uint16_t ADC_Read(ADC_Peripheral_t adc, ADC_Channel_t channel);

/**
 * @brief   Read ADC conversion as voltage in millivolts
 *
 * Converts the raw ADC value to millivolts using VREF as reference.
 *
 * @param   adc     ADC peripheral
 * @param   channel ADC input channel
 * @return  Voltage in millivolts (0-3300), or 0 on timeout
 */
uint32_t ADC_ReadMillivolts(ADC_Peripheral_t adc, ADC_Channel_t channel);

#endif /* HAL_ADC_H */

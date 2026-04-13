/**
 * @file    I2C.h
 * @brief   I2C (Inter-Integrated Circuit) HAL
 *
 * Provides a simple master-mode interface for I2C communication on the
 * STM32F103. Supports I2C1 and I2C2 with configurable speed.
 *
 * Default pin mapping (no remap):
 *   - I2C1: SCL = PB6, SDA = PB7
 *   - I2C2: SCL = PB10, SDA = PB11
 *
 * @note    Default system clock is 8 MHz HSI (no PLL).
 *          Adjust I2C_PCLK_HZ if using a different clock configuration.
 *
 * @version 1.0
 */

#ifndef HAL_I2C_H
#define HAL_I2C_H

#include <stdint.h>

/** @brief APB1 peripheral clock frequency in Hz (8 MHz HSI default) */
#define I2C_PCLK_HZ    8000000UL

/** @brief Default timeout for blocking operations (loop iterations) */
#define I2C_TIMEOUT     100000UL

/**
 * @enum I2C_Peripheral_t
 * @brief Available I2C peripherals
 */
typedef enum {
    I2C_1 = 0,  /**< I2C1 (APB1) - SCL:PB6  SDA:PB7  */
    I2C_2 = 1,  /**< I2C2 (APB1) - SCL:PB10 SDA:PB11 */
} I2C_Peripheral_t;

/**
 * @enum I2C_Speed_t
 * @brief I2C bus speed modes
 */
typedef enum {
    I2C_SPEED_100K = 100000,  /**< Standard mode  (100 kHz) */
    I2C_SPEED_400K = 400000,  /**< Fast mode      (400 kHz) */
} I2C_Speed_t;

/**
 * @brief   Initialize I2C peripheral in master mode
 *
 * Configures clock, GPIO pins (open-drain alternate function), and
 * timing registers for the selected speed.
 *
 * @param   i2c     I2C peripheral to initialize
 * @param   speed   Bus speed (100 kHz or 400 kHz)
 */
void I2C_Init(I2C_Peripheral_t i2c, I2C_Speed_t speed);

/**
 * @brief   Write data to an I2C slave device
 *
 * Generates START, sends slave address with write bit, transmits
 * data bytes, and generates STOP.
 *
 * @param   i2c     I2C peripheral
 * @param   addr    7-bit slave address (unshifted, e.g., 0x68 for MPU6050)
 * @param   data    Pointer to data buffer to transmit
 * @param   len     Number of bytes to write
 * @return  0 on success, -1 on error/timeout
 */
int8_t I2C_Write(I2C_Peripheral_t i2c, uint8_t addr,
                 const uint8_t *data, uint32_t len);

/**
 * @brief   Read data from an I2C slave device
 *
 * Generates START, sends slave address with read bit, receives
 * data bytes with ACK/NACK, and generates STOP.
 *
 * @param   i2c     I2C peripheral
 * @param   addr    7-bit slave address (unshifted)
 * @param   data    Pointer to buffer to store received data
 * @param   len     Number of bytes to read
 * @return  0 on success, -1 on error/timeout
 */
int8_t I2C_Read(I2C_Peripheral_t i2c, uint8_t addr,
                uint8_t *data, uint32_t len);

/**
 * @brief   Write a single byte to a register on an I2C slave
 *
 * Convenience function: sends register address followed by one data byte.
 *
 * @param   i2c     I2C peripheral
 * @param   addr    7-bit slave address (unshifted)
 * @param   reg     Register address on the slave device
 * @param   value   Byte value to write
 * @return  0 on success, -1 on error/timeout
 */
int8_t I2C_WriteReg(I2C_Peripheral_t i2c, uint8_t addr,
                    uint8_t reg, uint8_t value);

/**
 * @brief   Read a single byte from a register on an I2C slave
 *
 * Convenience function: sends register address, then reads one byte.
 *
 * @param   i2c     I2C peripheral
 * @param   addr    7-bit slave address (unshifted)
 * @param   reg     Register address on the slave device
 * @param   value   Pointer to store the read byte
 * @return  0 on success, -1 on error/timeout
 */
int8_t I2C_ReadReg(I2C_Peripheral_t i2c, uint8_t addr,
                   uint8_t reg, uint8_t *value);

#endif /* HAL_I2C_H */

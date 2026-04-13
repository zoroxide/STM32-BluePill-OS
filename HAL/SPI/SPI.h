/**
 * @file    SPI.h
 * @brief   SPI (Serial Peripheral Interface) HAL
 *
 * Provides a simple interface for SPI master communication on the
 * STM32F103. Supports SPI1 and SPI2 with configurable clock prescaler
 * and polarity/phase settings.
 *
 * Default pin mapping (no remap):
 *   - SPI1: SCK = PA5, MISO = PA6, MOSI = PA7
 *   - SPI2: SCK = PB13, MISO = PB14, MOSI = PB15
 *
 * @note    NSS (chip select) is managed by software using GPIO pins.
 *
 * @version 1.0
 */

#ifndef HAL_SPI_H
#define HAL_SPI_H

#include <stdint.h>

/** @brief Default timeout for blocking operations (loop iterations) */
#define SPI_TIMEOUT     100000UL

/**
 * @enum SPI_Peripheral_t
 * @brief Available SPI peripherals
 */
typedef enum {
    SPI_1 = 0,  /**< SPI1 (APB2) - SCK:PA5  MISO:PA6  MOSI:PA7  */
    SPI_2 = 1,  /**< SPI2 (APB1) - SCK:PB13 MISO:PB14 MOSI:PB15 */
} SPI_Peripheral_t;

/**
 * @enum SPI_Prescaler_t
 * @brief SPI clock prescaler values (PCLK / prescaler = SPI clock)
 */
typedef enum {
    SPI_PRESCALER_2   = 0,  /**< PCLK / 2   */
    SPI_PRESCALER_4   = 1,  /**< PCLK / 4   */
    SPI_PRESCALER_8   = 2,  /**< PCLK / 8   */
    SPI_PRESCALER_16  = 3,  /**< PCLK / 16  */
    SPI_PRESCALER_32  = 4,  /**< PCLK / 32  */
    SPI_PRESCALER_64  = 5,  /**< PCLK / 64  */
    SPI_PRESCALER_128 = 6,  /**< PCLK / 128 */
    SPI_PRESCALER_256 = 7,  /**< PCLK / 256 */
} SPI_Prescaler_t;

/**
 * @enum SPI_Mode_t
 * @brief SPI clock polarity/phase modes
 *
 * MODE 0: CPOL=0, CPHA=0 (clock idle low, sample on rising edge)
 * MODE 1: CPOL=0, CPHA=1 (clock idle low, sample on falling edge)
 * MODE 2: CPOL=1, CPHA=0 (clock idle high, sample on falling edge)
 * MODE 3: CPOL=1, CPHA=1 (clock idle high, sample on rising edge)
 */
typedef enum {
    SPI_MODE_0 = 0,  /**< CPOL=0, CPHA=0 */
    SPI_MODE_1 = 1,  /**< CPOL=0, CPHA=1 */
    SPI_MODE_2 = 2,  /**< CPOL=1, CPHA=0 */
    SPI_MODE_3 = 3,  /**< CPOL=1, CPHA=1 */
} SPI_Mode_t;

/**
 * @brief   Initialize SPI peripheral in master mode
 *
 * Configures clock, GPIO pins, prescaler, and polarity/phase.
 * Data frame format is 8-bit, MSB first, software NSS management.
 *
 * @param   spi         SPI peripheral to initialize
 * @param   prescaler   Clock prescaler value
 * @param   mode        SPI clock mode (polarity/phase)
 */
void SPI_Init(SPI_Peripheral_t spi, SPI_Prescaler_t prescaler, SPI_Mode_t mode);

/**
 * @brief   Transmit and receive a single byte (full-duplex)
 *
 * Sends one byte and simultaneously receives one byte.
 *
 * @param   spi     SPI peripheral
 * @param   tx_data Byte to transmit
 * @return  Received byte, or 0xFF on timeout
 */
uint8_t SPI_TransmitReceive8(SPI_Peripheral_t spi, uint8_t tx_data);

/**
 * @brief   Transmit a buffer of bytes
 *
 * Sends the specified number of bytes. Received data is discarded.
 *
 * @param   spi     SPI peripheral
 * @param   data    Pointer to transmit buffer
 * @param   len     Number of bytes to send
 */
void SPI_Transmit(SPI_Peripheral_t spi, const uint8_t *data, uint32_t len);

/**
 * @brief   Receive a buffer of bytes
 *
 * Transmits 0xFF dummy bytes to generate clock and reads incoming data.
 *
 * @param   spi     SPI peripheral
 * @param   data    Pointer to receive buffer
 * @param   len     Number of bytes to receive
 */
void SPI_Receive(SPI_Peripheral_t spi, uint8_t *data, uint32_t len);

/**
 * @brief   Full-duplex buffer transfer
 *
 * Simultaneously transmits from tx_buf and receives into rx_buf.
 *
 * @param   spi     SPI peripheral
 * @param   tx_buf  Pointer to transmit buffer
 * @param   rx_buf  Pointer to receive buffer
 * @param   len     Number of bytes to transfer
 */
void SPI_TransferBuffer(SPI_Peripheral_t spi, const uint8_t *tx_buf,
                        uint8_t *rx_buf, uint32_t len);

#endif /* HAL_SPI_H */

/**
 * @file    RC522.h
 * @brief   MFRC522 13.56 MHz RFID reader driver for STM32F103
 *
 * Drives an "RFID-RC522" (MFRC522) Arduino-style breakout over SPI.
 * Supports reading the 4-byte UID of Mifare Classic cards (ISO14443A),
 * which is the typical Arduino "scan a card" use case.
 *
 * @note  The MFRC522 chip supports SPI, I2C, and UART modes, but the
 *        Arduino module only exposes SPI.  SPI mode 0 (CPOL=0, CPHA=0),
 *        up to 10 MHz.  Bytes are MSB-first; the STM32 HAL default is
 *        compatible - no bit-reversal needed.
 *
 * Usage example:
 * @code
 *   RC522_Config_t cfg = {
 *       .spi      = SPI_2,
 *       .ss_port  = GPIO_PORT_B, .ss_pin  = GPIO_PIN_9,
 *       .rst_port = GPIO_PORT_B, .rst_pin = GPIO_PIN_8,
 *       .has_rst  = 1,
 *   };
 *   if (RC522_Init(&cfg) == 0) {
 *       uint8_t uid[RC522_UID_LEN];
 *       if (RC522_ReadUID(uid) == 0) {
 *           // uid is valid
 *       }
 *   }
 * @endcode
 *
 * @version 1.0
 */

#ifndef DRIVER_RC522_H
#define DRIVER_RC522_H

#include <stdint.h>
#include "HAL/SPI/SPI.h"
#include "HAL/IO/IO.h"

/*---------------------------------------------------------------------------*/
/* Constants                                                                 */
/*---------------------------------------------------------------------------*/

/** @brief 4-byte Mifare Classic UID (NUID; the single-size UID) */
#define RC522_UID_LEN           4

/** @brief Expected VersionReg values for genuine / clone MFRC522 silicon */
#define RC522_VERSION_1         0x91
#define RC522_VERSION_2         0x92
#define RC522_VERSION_CLONE     0xB2  /* some fakes */

/*---------------------------------------------------------------------------*/
/* Configuration                                                             */
/*---------------------------------------------------------------------------*/

/**
 * @struct RC522_Config_t
 * @brief  Runtime configuration for the MFRC522 driver
 */
typedef struct {
    SPI_Peripheral_t spi;      /**< SPI peripheral (e.g. SPI_2) */
    GPIO_Port_t      ss_port;  /**< Chip-select (SDA/NSS) port */
    GPIO_Pin_t       ss_pin;   /**< Chip-select pin */
    GPIO_Port_t      rst_port; /**< Hardware reset port (if wired) */
    GPIO_Pin_t       rst_pin;  /**< Hardware reset pin */
    uint8_t          has_rst;  /**< 1 if RST is wired, 0 to rely on soft reset */
} RC522_Config_t;

/*---------------------------------------------------------------------------*/
/* Public API                                                                */
/*---------------------------------------------------------------------------*/

/**
 * @brief   Initialize SPI, reset the MFRC522, and prepare it for polling
 *
 * Performs a hardware reset (if @c has_rst is set), then a soft reset,
 * configures the internal timer for a ~25 ms card-response timeout,
 * turns on the RF antenna, and verifies the chip version.
 *
 * @param[in] cfg  Populated configuration struct
 * @return  0 on success, -1 if the chip did not respond with a valid
 *          VersionReg value
 */
int8_t RC522_Init(const RC522_Config_t *cfg);

/**
 * @brief   Read the 8-bit VersionReg value
 *
 * Useful for diagnostics.  Genuine parts report 0x91 or 0x92; many
 * clones report 0xB2.  A value of 0x00 or 0xFF generally indicates
 * a wiring or power problem.
 *
 * @return  Raw VersionReg byte
 */
uint8_t RC522_GetVersion(void);

/**
 * @brief   Enable the RF antenna drivers
 */
void RC522_AntennaOn(void);

/**
 * @brief   Disable the RF antenna drivers
 */
void RC522_AntennaOff(void);

/**
 * @brief   Check whether a card is in the field (non-destructive)
 *
 * Sends a Mifare REQA request and returns 0 if any card responds.
 * Does not select/lock the card; repeated calls are safe.
 *
 * @return  0 if a card is detected, -1 if the field is empty
 */
int8_t RC522_IsCardPresent(void);

/**
 * @brief   Read the UID of a single card in the field
 *
 * Performs REQA followed by anticollision cascade level 1 and returns
 * the 4-byte UID of the first responding card.  The 5th byte (BCC) is
 * verified internally.
 *
 * @param[out] uid  Buffer of at least @ref RC522_UID_LEN bytes
 * @return  0 on success, -1 on timeout / no card / BCC mismatch
 */
int8_t RC522_ReadUID(uint8_t uid[RC522_UID_LEN]);

/**
 * @brief   Send a HALT (PICC_HALT) command to the currently-selected card
 *
 * Puts the card into HALT state so that a subsequent REQA (not WUPA)
 * will not return it again - useful to debounce scan loops.
 */
void RC522_Halt(void);

#endif /* DRIVER_RC522_H */

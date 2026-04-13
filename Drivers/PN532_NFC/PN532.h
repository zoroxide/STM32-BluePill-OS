/**
 * @file    PN532.h
 * @brief   PN532 NFC/RFID module driver for STM32F103
 *
 * Full-featured driver for the NXP PN532 NFC controller supporting all
 * three communication interfaces: SPI, I2C, and UART.  Provides high-level
 * helpers for Mifare Classic tag operations as well as low-level command /
 * response access for custom NFC workflows.
 *
 * Usage example (I2C):
 * @code
 *   PN532_Config_t cfg = {
 *       .transport = PN532_TRANSPORT_I2C,
 *       .i2c       = I2C_1,
 *   };
 *   PN532_Init(&cfg);
 *
 *   uint8_t uid[PN532_MAX_UID_LEN];
 *   uint8_t uid_len;
 *   if (PN532_ReadPassiveTarget(PN532_BAUD_106, uid, &uid_len) == 0) {
 *       // Card detected - uid/uid_len are valid
 *   }
 * @endcode
 *
 * @version 1.0
 */

#ifndef DRIVER_PN532_H
#define DRIVER_PN532_H

#include <stdint.h>
#include "HAL/SPI/SPI.h"
#include "HAL/I2C/I2C.h"
#include "HAL/UART/UART.h"
#include "HAL/IO/IO.h"

/*---------------------------------------------------------------------------*/
/* PN532 Protocol Constants                                                  */
/*---------------------------------------------------------------------------*/

/** @brief 7-bit I2C slave address of the PN532 */
#define PN532_I2C_ADDR              0x24

/** @brief ACK frame returned by the PN532 after a valid host command */
#define PN532_ACK                   {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00}

/** @brief Preamble byte */
#define PN532_PREAMBLE              0x00
/** @brief Start code byte 1 */
#define PN532_STARTCODE1            0x00
/** @brief Start code byte 2 */
#define PN532_STARTCODE2            0xFF

/** @brief Transport Frame Identifier: host to PN532 */
#define PN532_HOSTTOPN532           0xD4
/** @brief Transport Frame Identifier: PN532 to host */
#define PN532_PN532TOHOST           0xD5

/** @brief Default command/response timeout in milliseconds */
#define PN532_TIMEOUT               1000

/*---------------------------------------------------------------------------*/
/* SPI-Specific Command Bytes                                                */
/*---------------------------------------------------------------------------*/

/** @brief SPI: data write command */
#define PN532_SPI_DATAWRITE         0x01
/** @brief SPI: status read command */
#define PN532_SPI_STATREAD          0x02
/** @brief SPI: data read command */
#define PN532_SPI_DATAREAD          0x03

/*---------------------------------------------------------------------------*/
/* PN532 Command Codes                                                       */
/*---------------------------------------------------------------------------*/

/** @brief Get IC, firmware version, and feature support */
#define PN532_CMD_GETFIRMWAREVERSION    0x02
/** @brief Configure the Security Access Module */
#define PN532_CMD_SAMCONFIGURATION      0x14
/** @brief Detect and list passive targets (cards) in the RF field */
#define PN532_CMD_INLISTPASSIVETARGET   0x4A
/** @brief Exchange data with an activated target */
#define PN532_CMD_INDATAEXCHANGE        0x40

/** @brief Mifare Classic: read 16-byte block */
#define PN532_CMD_MIFARE_READ           0x30
/** @brief Mifare Classic: write 16-byte block */
#define PN532_CMD_MIFARE_WRITE          0xA0
/** @brief Mifare Classic: authenticate with Key A */
#define PN532_CMD_MIFARE_AUTH_A         0x60
/** @brief Mifare Classic: authenticate with Key B */
#define PN532_CMD_MIFARE_AUTH_B         0x61

/*---------------------------------------------------------------------------*/
/* Card Detection Parameters                                                 */
/*---------------------------------------------------------------------------*/

/** @brief Baud rate code for ISO14443 Type A (106 kbps) */
#define PN532_BAUD_106              0x00

/*---------------------------------------------------------------------------*/
/* Buffer Size Limits                                                        */
/*---------------------------------------------------------------------------*/

/** @brief Maximum UID length in bytes (ISO14443A triple-size UID) */
#define PN532_MAX_UID_LEN           10

/** @brief Maximum payload data length for command/response frames */
#define PN532_MAX_DATA_LEN          64

/*---------------------------------------------------------------------------*/
/* Transport Selection                                                       */
/*---------------------------------------------------------------------------*/

/**
 * @enum PN532_Transport_t
 * @brief Communication interface used to talk to the PN532
 */
typedef enum {
    PN532_TRANSPORT_SPI  = 0,  /**< SPI interface */
    PN532_TRANSPORT_I2C  = 1,  /**< I2C interface */
    PN532_TRANSPORT_UART = 2,  /**< UART interface */
} PN532_Transport_t;

/*---------------------------------------------------------------------------*/
/* Configuration                                                             */
/*---------------------------------------------------------------------------*/

/**
 * @struct PN532_Config_t
 * @brief  Runtime configuration for the PN532 driver
 *
 * Populate the @c transport field and the matching peripheral fields
 * before calling @ref PN532_Init.  Unused interface fields are ignored.
 */
typedef struct {
    PN532_Transport_t transport;  /**< Selected communication interface */

    /* SPI configuration */
    SPI_Peripheral_t  spi;        /**< SPI peripheral (e.g. SPI_1) */
    GPIO_Port_t       ss_port;    /**< Chip-select GPIO port */
    GPIO_Pin_t        ss_pin;     /**< Chip-select GPIO pin */

    /* I2C configuration */
    I2C_Peripheral_t  i2c;        /**< I2C peripheral (e.g. I2C_1) */

    /* UART configuration */
    UART_Peripheral_t uart;       /**< UART peripheral (e.g. UART_2) */
} PN532_Config_t;

/*---------------------------------------------------------------------------*/
/* Public API - Core                                                         */
/*---------------------------------------------------------------------------*/

/**
 * @brief   Initialize the PN532 module
 *
 * Sets up the selected transport peripheral, wakes the PN532 from
 * low-power mode, and runs the SAMConfiguration command so the module
 * is ready for card operations.
 *
 * @param[in] config  Pointer to a populated configuration struct
 * @return  0 on success, -1 on error
 */
int8_t PN532_Init(const PN532_Config_t *config);

/**
 * @brief   Read PN532 firmware version information
 *
 * Queries the module for its IC type, firmware version, revision, and
 * supported feature flags.
 *
 * @param[out] ic       IC type (e.g. 0x07 for PN532)
 * @param[out] ver      Firmware version number
 * @param[out] rev      Firmware revision number
 * @param[out] support  Feature support bitmask
 * @return  0 on success, -1 on error
 */
int8_t PN532_GetFirmwareVersion(uint8_t *ic, uint8_t *ver,
                                uint8_t *rev, uint8_t *support);

/*---------------------------------------------------------------------------*/
/* Public API - Card Detection                                               */
/*---------------------------------------------------------------------------*/

/**
 * @brief   Detect and read one passive NFC target
 *
 * Activates the RF field and waits for a card to enter range.  Blocks
 * until a card is found or @ref PN532_TIMEOUT milliseconds elapse.
 *
 * @param[in]  baud      Baud rate code (use @ref PN532_BAUD_106 for ISO14443A)
 * @param[out] uid       Buffer to receive the card UID (must be at least
 *                        @ref PN532_MAX_UID_LEN bytes)
 * @param[out] uid_len   Number of valid bytes written to @p uid
 * @return  0 if a card was found, -1 on timeout or error
 */
int8_t PN532_ReadPassiveTarget(uint8_t baud, uint8_t *uid, uint8_t *uid_len);

/**
 * @brief   Quick check for card presence
 *
 * Non-blocking convenience wrapper that returns immediately after a
 * short poll cycle.
 *
 * @return  1 if a card is present, 0 if not
 */
int8_t PN532_IsCardPresent(void);

/*---------------------------------------------------------------------------*/
/* Public API - Mifare Classic Operations                                    */
/*---------------------------------------------------------------------------*/

/**
 * @brief   Authenticate a Mifare Classic block with Key A
 *
 * Must be called before reading or writing a protected block.
 *
 * @param[in] block    Block number to authenticate
 * @param[in] key      Pointer to 6-byte key
 * @param[in] uid      Pointer to the card UID
 * @param[in] uid_len  Length of the UID in bytes
 * @return  0 on success, -1 on authentication failure
 */
int8_t PN532_MifareAuthA(uint8_t block, const uint8_t *key,
                         const uint8_t *uid, uint8_t uid_len);

/**
 * @brief   Authenticate a Mifare Classic block with Key B
 *
 * Must be called before reading or writing a protected block.
 *
 * @param[in] block    Block number to authenticate
 * @param[in] key      Pointer to 6-byte key
 * @param[in] uid      Pointer to the card UID
 * @param[in] uid_len  Length of the UID in bytes
 * @return  0 on success, -1 on authentication failure
 */
int8_t PN532_MifareAuthB(uint8_t block, const uint8_t *key,
                         const uint8_t *uid, uint8_t uid_len);

/**
 * @brief   Read a 16-byte Mifare Classic block
 *
 * The target block must be authenticated first using
 * @ref PN532_MifareAuthA or @ref PN532_MifareAuthB.
 *
 * @param[in]  block  Block number to read
 * @param[out] data   Buffer to receive the 16-byte block (must be >= 16 bytes)
 * @return  0 on success, -1 on error
 */
int8_t PN532_MifareRead(uint8_t block, uint8_t *data);

/**
 * @brief   Write a 16-byte Mifare Classic block
 *
 * The target block must be authenticated first using
 * @ref PN532_MifareAuthA or @ref PN532_MifareAuthB.
 *
 * @param[in] block  Block number to write
 * @param[in] data   Pointer to 16 bytes of data to write
 * @return  0 on success, -1 on error
 */
int8_t PN532_MifareWrite(uint8_t block, const uint8_t *data);

/*---------------------------------------------------------------------------*/
/* Public API - Low-Level                                                    */
/*---------------------------------------------------------------------------*/

/**
 * @brief   Send a raw command frame to the PN532
 *
 * Builds the full transport frame (preamble, length, TFI, checksum) and
 * transmits it over the configured interface.  The caller supplies only
 * the command byte(s) after TFI.
 *
 * @param[in] cmd      Pointer to command bytes (starting with the command code)
 * @param[in] cmd_len  Number of command bytes
 * @return  0 on success (ACK received), -1 on error
 */
int8_t PN532_SendCommand(const uint8_t *cmd, uint8_t cmd_len);

/**
 * @brief   Read a raw response frame from the PN532
 *
 * Waits for the PN532 to become ready, then reads the response frame
 * and extracts the payload (everything after TFI, before checksum).
 *
 * @param[out] buf       Buffer to store the response payload
 * @param[in]  buf_len   Maximum number of bytes to store
 * @param[out] resp_len  Actual number of payload bytes received
 * @return  0 on success, -1 on error or timeout
 */
int8_t PN532_ReadResponse(uint8_t *buf, uint8_t buf_len, uint8_t *resp_len);

/**
 * @brief   Wait until the PN532 signals it is ready
 *
 * Polls the ready status using the method appropriate for the active
 * transport (SPI status read, I2C ready byte, or UART ACK).
 *
 * @param[in] timeout_ms  Maximum time to wait in milliseconds
 * @return  0 when ready, -1 on timeout
 */
int8_t PN532_WaitReady(uint32_t timeout_ms);

#endif /* DRIVER_PN532_H */

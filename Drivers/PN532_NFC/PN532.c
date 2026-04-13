/**
 * @file    PN532.c
 * @brief   PN532 NFC/RFID module driver implementation for STM32F103
 *
 * Supports SPI, I2C, and UART transports.  All public functions return
 * 0 on success and -1 on error.  No dynamic memory allocation is used.
 *
 * @version 1.0
 */

#include "PN532.h"
#include "HAL/ISR/ISR.h"

/*---------------------------------------------------------------------------*/
/* Embedded-safe helpers (no stdlib)                                          */
/*---------------------------------------------------------------------------*/
#ifndef NULL
#define NULL ((void *)0)
#endif

static void pn532_memcpy(uint8_t *dst, const uint8_t *src, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n; i++) { dst[i] = src[i]; }
}

static uint8_t pn532_memcmp(const uint8_t *a, const uint8_t *b, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n; i++) { if (a[i] != b[i]) return 1; }
    return 0;
}

/*---------------------------------------------------------------------------*/
/* Module State                                                              */
/*---------------------------------------------------------------------------*/

/** @brief Stored driver configuration (set once during PN532_Init) */
static PN532_Config_t pn532_cfg;

/** @brief Scratch buffer for building and parsing frames */
static uint8_t pn532_buf[PN532_MAX_DATA_LEN + 10];

/*---------------------------------------------------------------------------*/
/* SPI Bit-Reversal Helper                                                   */
/*---------------------------------------------------------------------------*/

/**
 * @brief  Reverse the bit order of a single byte
 *
 * The PN532 SPI interface is LSB-first, but the STM32 SPI HAL sends
 * MSB-first.  Every byte must be bit-reversed before transmit and
 * after receive.
 *
 * @param[in] b  Byte to reverse
 * @return       Bit-reversed byte
 */
static uint8_t reverse_byte(uint8_t b)
{
    b = ((b >> 1) & 0x55) | ((b & 0x55) << 1);
    b = ((b >> 2) & 0x33) | ((b & 0x33) << 2);
    return (b >> 4) | (b << 4);
}

/*---------------------------------------------------------------------------*/
/* SPI Transport Helpers                                                     */
/*---------------------------------------------------------------------------*/

/**
 * @brief  Assert (pull LOW) the SPI chip-select line
 */
static void spi_ss_low(void)
{
    GPIO_ClrPin(pn532_cfg.ss_port, pn532_cfg.ss_pin);
}

/**
 * @brief  De-assert (pull HIGH) the SPI chip-select line
 */
static void spi_ss_high(void)
{
    GPIO_SetPin(pn532_cfg.ss_port, pn532_cfg.ss_pin);
}

/**
 * @brief  Send a frame over SPI with bit-reversal
 *
 * Pulls SS LOW, sends the SPI data-write command byte, then sends
 * each frame byte (all bit-reversed).  Pulls SS HIGH when done.
 *
 * @param[in] data  Frame bytes to send
 * @param[in] len   Number of bytes
 */
static void spi_write_frame(const uint8_t *data, uint8_t len)
{
    spi_ss_low();
    (void)SPI_TransmitReceive8(pn532_cfg.spi, reverse_byte(PN532_SPI_DATAWRITE));
    for (uint8_t i = 0; i < len; i++) {
        (void)SPI_TransmitReceive8(pn532_cfg.spi, reverse_byte(data[i]));
    }
    spi_ss_high();
}

/**
 * @brief  Read a frame over SPI with bit-reversal
 *
 * Pulls SS LOW, sends the SPI data-read command byte, then clocks
 * out @p len bytes (sending 0x00 as dummy, bit-reversing the result).
 * Pulls SS HIGH when done.
 *
 * @param[out] data  Buffer to receive frame bytes
 * @param[in]  len   Number of bytes to read
 */
static void spi_read_frame(uint8_t *data, uint8_t len)
{
    spi_ss_low();
    (void)SPI_TransmitReceive8(pn532_cfg.spi, reverse_byte(PN532_SPI_DATAREAD));
    for (uint8_t i = 0; i < len; i++) {
        data[i] = reverse_byte(SPI_TransmitReceive8(pn532_cfg.spi, 0x00));
    }
    spi_ss_high();
}

/**
 * @brief  Check SPI ready status
 *
 * @return 1 if PN532 is ready, 0 otherwise
 */
static uint8_t spi_is_ready(void)
{
    spi_ss_low();
    (void)SPI_TransmitReceive8(pn532_cfg.spi, reverse_byte(PN532_SPI_STATREAD));
    uint8_t status = reverse_byte(SPI_TransmitReceive8(pn532_cfg.spi, 0x00));
    spi_ss_high();
    return (status & 0x01) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
/* I2C Transport Helpers                                                     */
/*---------------------------------------------------------------------------*/

/**
 * @brief  Check I2C ready status
 *
 * Reads a single byte from the PN532 I2C address.  The module returns
 * 0x01 when it has data ready.
 *
 * @return 1 if ready, 0 otherwise
 */
static uint8_t i2c_is_ready(void)
{
    uint8_t status = 0;
    if (I2C_Read(pn532_cfg.i2c, PN532_I2C_ADDR, &status, 1) != 0) {
        return 0;
    }
    return (status == 0x01) ? 1 : 0;
}

/**
 * @brief  Send a frame over I2C
 *
 * @param[in] data  Frame bytes to send
 * @param[in] len   Number of bytes
 */
static void i2c_write_frame(const uint8_t *data, uint8_t len)
{
    (void)I2C_Write(pn532_cfg.i2c, PN532_I2C_ADDR, data, len);
}

/**
 * @brief  Read a frame over I2C
 *
 * The first byte from the PN532 is the ready indicator (0x01) and is
 * stripped.  The remaining @p len bytes are the actual frame data.
 *
 * @param[out] data  Buffer to receive frame bytes
 * @param[in]  len   Number of frame bytes expected
 */
static void i2c_read_frame(uint8_t *data, uint8_t len)
{
    uint8_t tmp[PN532_MAX_DATA_LEN + 11];
    (void)I2C_Read(pn532_cfg.i2c, PN532_I2C_ADDR, tmp, (uint8_t)(len + 1));
    /* Skip the ready indicator byte (tmp[0]) */
    pn532_memcpy((uint8_t *)data, &tmp[1], len);
}

/*---------------------------------------------------------------------------*/
/* UART Transport Helpers                                                    */
/*---------------------------------------------------------------------------*/

/**
 * @brief  Send the UART wakeup sequence
 *
 * Transmits 0x55 repeated bytes followed by padding zeros and waits
 * 50 ms for the PN532 to wake up from power-down.
 */
/**
 * @brief  Drain any pending bytes from the UART RX buffer
 */
static void uart_drain(void)
{
    uint8_t dummy;
    uint16_t guard = 1000;
    while (UART_IsRxReady(pn532_cfg.uart) && --guard) {
        UART_ReceiveByte(pn532_cfg.uart, &dummy);
    }
}

static void uart_wakeup(void)
{
    /* Send long preamble of 0x55 to wake PN532 from power-down */
    uint8_t i;
    for (i = 0; i < 24; i++) {
        UART_SendByte(pn532_cfg.uart, 0x55);
    }
    /* End with 0x00 to mark end of wakeup */
    UART_SendByte(pn532_cfg.uart, 0x00);

    /* Wait for PN532 to fully wake up */
    SysTick_DelayMs(100);

    /* Drain any response/garbage bytes */
    uart_drain();
}

/**
 * @brief  Send a frame over UART
 *
 * @param[in] data  Frame bytes to send
 * @param[in] len   Number of bytes
 */
static void uart_write_frame(const uint8_t *data, uint8_t len)
{
    UART_SendBuffer(pn532_cfg.uart, data, len);
}

/**
 * @brief  Read bytes over UART
 *
 * @param[out] data  Buffer to receive bytes
 * @param[in]  len   Number of bytes to read
 * @return  0 on success, -1 on timeout
 */
static int8_t uart_read_frame(uint8_t *data, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        if (UART_ReceiveByte(pn532_cfg.uart, &data[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

/**
 * @brief  Check UART ready status
 *
 * @return 1 if data is available, 0 otherwise
 */
static uint8_t uart_is_ready(void)
{
    return UART_IsRxReady(pn532_cfg.uart) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
/* Generic Transport Wrappers                                                */
/*---------------------------------------------------------------------------*/

/**
 * @brief  Send a frame via the active transport
 *
 * @param[in] data  Frame bytes
 * @param[in] len   Frame length
 */
static void transport_write(const uint8_t *data, uint8_t len)
{
    switch (pn532_cfg.transport) {
        case PN532_TRANSPORT_SPI:
            spi_write_frame(data, len);
            break;
        case PN532_TRANSPORT_I2C:
            i2c_write_frame(data, len);
            break;
        case PN532_TRANSPORT_UART:
            uart_write_frame(data, len);
            break;
    }
}

/**
 * @brief  Read a frame via the active transport
 *
 * @param[out] data  Buffer for frame bytes
 * @param[in]  len   Number of bytes to read
 * @return  0 on success, -1 on error
 */
static int8_t transport_read(uint8_t *data, uint8_t len)
{
    switch (pn532_cfg.transport) {
        case PN532_TRANSPORT_SPI:
            spi_read_frame(data, len);
            return 0;
        case PN532_TRANSPORT_I2C:
            i2c_read_frame(data, len);
            return 0;
        case PN532_TRANSPORT_UART:
            return uart_read_frame(data, len);
        default:
            return -1;
    }
}

/**
 * @brief  Check if the PN532 is ready via the active transport
 *
 * @return 1 if ready, 0 otherwise
 */
static uint8_t transport_is_ready(void)
{
    switch (pn532_cfg.transport) {
        case PN532_TRANSPORT_SPI:  return spi_is_ready();
        case PN532_TRANSPORT_I2C:  return i2c_is_ready();
        case PN532_TRANSPORT_UART: return uart_is_ready();
        default:                   return 0;
    }
}

/*---------------------------------------------------------------------------*/
/* Frame Building Helper                                                     */
/*---------------------------------------------------------------------------*/

/**
 * @brief  Build a PN532 command frame in the module scratch buffer
 *
 * Constructs the full frame: preamble, start codes, length, length
 * checksum, TFI (0xD4), command bytes, data checksum, and postamble.
 *
 * @param[in]  cmd      Command bytes (starting with the command code)
 * @param[in]  cmd_len  Number of command bytes
 * @param[out] frame_len  Total frame length written to pn532_buf
 */
static void build_frame(const uint8_t *cmd, uint8_t cmd_len,
                         uint8_t *frame_len)
{
    uint8_t len = cmd_len + 1;  /* TFI + command bytes */
    uint8_t lcs = (~len + 1) & 0xFF;
    uint8_t idx = 0;

    pn532_buf[idx++] = PN532_PREAMBLE;
    pn532_buf[idx++] = PN532_STARTCODE1;
    pn532_buf[idx++] = PN532_STARTCODE2;
    pn532_buf[idx++] = len;
    pn532_buf[idx++] = lcs;
    pn532_buf[idx++] = PN532_HOSTTOPN532;

    uint8_t dcs_sum = PN532_HOSTTOPN532;
    for (uint8_t i = 0; i < cmd_len; i++) {
        pn532_buf[idx++] = cmd[i];
        dcs_sum += cmd[i];
    }

    pn532_buf[idx++] = (~dcs_sum + 1) & 0xFF;
    pn532_buf[idx++] = 0x00;  /* Postamble */

    *frame_len = idx;
}

/*---------------------------------------------------------------------------*/
/* ACK Verification                                                          */
/*---------------------------------------------------------------------------*/

/**
 * @brief  Read and verify a PN532 ACK frame
 *
 * @return 0 if valid ACK received, -1 on error
 */
static int8_t read_ack(void)
{
    uint8_t ack[6];
    const uint8_t expected[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};

    if (transport_read(ack, 6) != 0) {
        return -1;
    }

    if (pn532_memcmp((const uint8_t *)ack, expected, 6) != 0) {
        return -1;
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/* Mifare Authentication Helper                                              */
/*---------------------------------------------------------------------------*/

/**
 * @brief  Perform Mifare Classic authentication
 *
 * Sends an InDataExchange command with the specified authentication
 * type (Key A or Key B), block number, key, and card UID.
 *
 * @param[in] auth_cmd  PN532_CMD_MIFARE_AUTH_A or PN532_CMD_MIFARE_AUTH_B
 * @param[in] block     Block number to authenticate
 * @param[in] key       Pointer to 6-byte key
 * @param[in] uid       Pointer to the card UID
 * @param[in] uid_len   Length of the UID in bytes
 * @return  0 on success, -1 on error
 */
static int8_t mifare_auth(uint8_t auth_cmd, uint8_t block,
                           const uint8_t *key, const uint8_t *uid,
                           uint8_t uid_len)
{
    /* InDataExchange: Tg=1, AUTH_CMD, block, key[6], uid[uid_len] */
    uint8_t cmd[3 + 6 + PN532_MAX_UID_LEN];
    uint8_t idx = 0;

    cmd[idx++] = PN532_CMD_INDATAEXCHANGE;
    cmd[idx++] = 0x01;       /* Target number */
    cmd[idx++] = auth_cmd;   /* Auth type */
    cmd[idx++] = block;

    /* 6-byte key */
    pn532_memcpy((uint8_t *)&cmd[idx], key, 6);
    idx += 6;

    /* UID */
    if (uid_len > PN532_MAX_UID_LEN) {
        return -1;
    }
    pn532_memcpy((uint8_t *)&cmd[idx], uid, uid_len);
    idx += uid_len;

    if (PN532_SendCommand(cmd, idx) != 0) {
        return -1;
    }

    uint8_t resp[2];
    uint8_t resp_len = 0;
    if (PN532_ReadResponse(resp, sizeof(resp), &resp_len) != 0) {
        return -1;
    }

    /* Response: CMD+1 byte already stripped; first byte is status */
    if (resp_len < 1 || resp[0] != 0x00) {
        return -1;
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/* Public API Implementation                                                 */
/*---------------------------------------------------------------------------*/

/**
 * @brief   Initialize the PN532 module
 */
int8_t PN532_Init(const PN532_Config_t *config)
{
    if (config == NULL) {
        return -1;
    }

    pn532_memcpy((uint8_t *)&pn532_cfg, (const uint8_t *)config, sizeof(PN532_Config_t));

    /* Initialize the selected transport */
    switch (pn532_cfg.transport) {
        case PN532_TRANSPORT_SPI:
            SPI_Init(pn532_cfg.spi, SPI_PRESCALER_64, SPI_MODE_0);
            GPIO_Init(pn532_cfg.ss_port, pn532_cfg.ss_pin);
            spi_ss_high();
            SysTick_DelayMs(100);
            break;

        case PN532_TRANSPORT_I2C:
            /* I2C peripheral assumed already initialized by caller */
            break;

        case PN532_TRANSPORT_UART:
            UART_Init(pn532_cfg.uart, 115200);
            SysTick_DelayMs(50);
            uart_wakeup();
            break;

        default:
            return -1;
    }

    SysTick_DelayMs(100);

    /* SAMConfiguration: Normal mode (0x01), timeout (0x14), IRQ off (0x00) */
    uint8_t sam_cmd[] = {PN532_CMD_SAMCONFIGURATION, 0x01, 0x14, 0x00};
    if (PN532_SendCommand(sam_cmd, sizeof(sam_cmd)) != 0) {
        return -1;
    }

    uint8_t resp[1];
    uint8_t resp_len = 0;
    if (PN532_ReadResponse(resp, sizeof(resp), &resp_len) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief   Send a raw command frame to the PN532
 */
int8_t PN532_SendCommand(const uint8_t *cmd, uint8_t cmd_len)
{
    if (cmd == NULL || cmd_len == 0) {
        return -1;
    }

    /* Drain any leftover RX data before sending */
    if (pn532_cfg.transport == PN532_TRANSPORT_UART) {
        uart_drain();
    }

    uint8_t frame_len = 0;
    build_frame(cmd, cmd_len, &frame_len);
    transport_write(pn532_buf, frame_len);

    /* Small delay to let PN532 start processing */
    SysTick_DelayMs(5);

    return 0;
}

/**
 * @brief   Wait until the PN532 signals it is ready
 */
int8_t PN532_WaitReady(uint32_t timeout_ms)
{
    uint32_t start = SysTick_GetTicks();

    while ((SysTick_GetTicks() - start) < timeout_ms) {
        if (transport_is_ready()) {
            return 0;
        }
        SysTick_DelayMs(1);
    }

    return -1;
}

/**
 * @brief   Read a raw response frame from the PN532
 */
int8_t PN532_ReadResponse(uint8_t *buf, uint8_t buf_len, uint8_t *resp_len)
{
    if (buf == NULL || resp_len == NULL) {
        return -1;
    }

    *resp_len = 0;

    /* Wait for ready, then read and verify ACK */
    SysTick_DelayMs(1);
    if (PN532_WaitReady(PN532_TIMEOUT) != 0) {
        return -1;
    }

    if (read_ack() != 0) {
        return -1;
    }

    /* Wait for ready again before reading the response frame */
    if (PN532_WaitReady(PN532_TIMEOUT) != 0) {
        return -1;
    }

    /* Read the frame header: preamble(1) + startcodes(2) + len(1) + lcs(1) = 5 bytes */
    uint8_t header[5];
    if (transport_read(header, 5) != 0) {
        return -1;
    }

    /* Validate preamble and start codes */
    if (header[0] != PN532_PREAMBLE ||
        header[1] != PN532_STARTCODE1 ||
        header[2] != PN532_STARTCODE2) {
        return -1;
    }

    uint8_t data_len = header[3];
    uint8_t lcs = header[4];

    /* Verify length checksum */
    if (((data_len + lcs) & 0xFF) != 0) {
        return -1;
    }

    if (data_len == 0 || data_len > PN532_MAX_DATA_LEN + 2) {
        return -1;
    }

    /* Read data bytes + DCS + postamble */
    uint8_t body[PN532_MAX_DATA_LEN + 4];
    if (transport_read(body, (uint8_t)(data_len + 2)) != 0) {
        return -1;
    }

    /* Verify data checksum */
    uint8_t dcs_sum = 0;
    for (uint8_t i = 0; i < data_len; i++) {
        dcs_sum += body[i];
    }
    dcs_sum += body[data_len]; /* DCS byte */
    if ((dcs_sum & 0xFF) != 0) {
        return -1;
    }

    /* Verify TFI */
    if (body[0] != PN532_PN532TOHOST) {
        return -1;
    }

    /* Extract payload (skip TFI byte, include command response code and data) */
    uint8_t payload_len = data_len - 1; /* Exclude TFI */
    if (payload_len > buf_len) {
        payload_len = buf_len;
    }

    pn532_memcpy((uint8_t *)buf, &body[1], payload_len);
    *resp_len = payload_len;

    return 0;
}

/**
 * @brief   Read PN532 firmware version information
 */
int8_t PN532_GetFirmwareVersion(uint8_t *ic, uint8_t *ver,
                                uint8_t *rev, uint8_t *support)
{
    if (ic == NULL || ver == NULL || rev == NULL || support == NULL) {
        return -1;
    }

    uint8_t cmd[] = {PN532_CMD_GETFIRMWAREVERSION};
    if (PN532_SendCommand(cmd, sizeof(cmd)) != 0) {
        return -1;
    }

    uint8_t resp[5];
    uint8_t resp_len = 0;
    if (PN532_ReadResponse(resp, sizeof(resp), &resp_len) != 0) {
        return -1;
    }

    /* Response: [CMD+1, IC, Ver, Rev, Support] */
    if (resp_len < 5) {
        return -1;
    }

    *ic      = resp[1];
    *ver     = resp[2];
    *rev     = resp[3];
    *support = resp[4];

    return 0;
}

/**
 * @brief   Detect and read one passive NFC target
 */
int8_t PN532_ReadPassiveTarget(uint8_t baud, uint8_t *uid, uint8_t *uid_len)
{
    if (uid == NULL || uid_len == NULL) {
        return -1;
    }

    *uid_len = 0;

    /* InListPassiveTarget: max 1 target, specified baud rate */
    uint8_t cmd[] = {PN532_CMD_INLISTPASSIVETARGET, 0x01, baud};
    if (PN532_SendCommand(cmd, sizeof(cmd)) != 0) {
        return -1;
    }

    uint8_t resp[PN532_MAX_DATA_LEN];
    uint8_t resp_len = 0;
    if (PN532_ReadResponse(resp, sizeof(resp), &resp_len) != 0) {
        return -1;
    }

    /*
     * Response format (ISO14443A):
     * [CMD+1, NbTg, Tg, SENS_RES(2), SEL_RES, NFCIDLength, NFCID...]
     *  [0]     [1]  [2]  [3..4]       [5]      [6]          [7...]
     */
    if (resp_len < 2) {
        return -1;
    }

    uint8_t nb_targets = resp[1];
    if (nb_targets == 0) {
        return -1;
    }

    if (resp_len < 8) {
        return -1;
    }

    uint8_t nfcid_len = resp[6];
    if (nfcid_len > PN532_MAX_UID_LEN || resp_len < (uint8_t)(7 + nfcid_len)) {
        return -1;
    }

    pn532_memcpy((uint8_t *)uid, &resp[7], nfcid_len);
    *uid_len = nfcid_len;

    return 0;
}

/**
 * @brief   Quick check for card presence
 */
int8_t PN532_IsCardPresent(void)
{
    uint8_t uid[PN532_MAX_UID_LEN];
    uint8_t uid_len = 0;

    if (PN532_ReadPassiveTarget(PN532_BAUD_106, uid, &uid_len) == 0) {
        return 1;
    }

    return 0;
}

/**
 * @brief   Authenticate a Mifare Classic block with Key A
 */
int8_t PN532_MifareAuthA(uint8_t block, const uint8_t *key,
                         const uint8_t *uid, uint8_t uid_len)
{
    return mifare_auth(PN532_CMD_MIFARE_AUTH_A, block, key, uid, uid_len);
}

/**
 * @brief   Authenticate a Mifare Classic block with Key B
 */
int8_t PN532_MifareAuthB(uint8_t block, const uint8_t *key,
                         const uint8_t *uid, uint8_t uid_len)
{
    return mifare_auth(PN532_CMD_MIFARE_AUTH_B, block, key, uid, uid_len);
}

/**
 * @brief   Read a 16-byte Mifare Classic block
 */
int8_t PN532_MifareRead(uint8_t block, uint8_t *data)
{
    if (data == NULL) {
        return -1;
    }

    /* InDataExchange: Tg=1, MIFARE_READ, block */
    uint8_t cmd[] = {PN532_CMD_INDATAEXCHANGE, 0x01,
                     PN532_CMD_MIFARE_READ, block};
    if (PN532_SendCommand(cmd, sizeof(cmd)) != 0) {
        return -1;
    }

    uint8_t resp[18];
    uint8_t resp_len = 0;
    if (PN532_ReadResponse(resp, sizeof(resp), &resp_len) != 0) {
        return -1;
    }

    /* Response: [CMD+1, Status, Data[16]] */
    if (resp_len < 18 || resp[1] != 0x00) {
        return -1;
    }

    pn532_memcpy((uint8_t *)data, &resp[2], 16);

    return 0;
}

/**
 * @brief   Write a 16-byte Mifare Classic block
 */
int8_t PN532_MifareWrite(uint8_t block, const uint8_t *data)
{
    if (data == NULL) {
        return -1;
    }

    /* InDataExchange: Tg=1, MIFARE_WRITE, block, data[16] */
    uint8_t cmd[4 + 16];
    cmd[0] = PN532_CMD_INDATAEXCHANGE;
    cmd[1] = 0x01;
    cmd[2] = PN532_CMD_MIFARE_WRITE;
    cmd[3] = block;
    pn532_memcpy((uint8_t *)&cmd[4], data, 16);

    if (PN532_SendCommand(cmd, sizeof(cmd)) != 0) {
        return -1;
    }

    uint8_t resp[2];
    uint8_t resp_len = 0;
    if (PN532_ReadResponse(resp, sizeof(resp), &resp_len) != 0) {
        return -1;
    }

    /* Response: [CMD+1, Status] - status 0x00 = success */
    if (resp_len < 2 || resp[1] != 0x00) {
        return -1;
    }

    return 0;
}

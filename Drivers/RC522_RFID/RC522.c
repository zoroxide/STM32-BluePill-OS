/**
 * @file    RC522.c
 * @brief   MFRC522 RFID reader driver implementation for STM32F103
 *
 * ISO14443A card detection over SPI.  Supports REQA + single-cascade
 * anticollision to recover a 4-byte Mifare Classic UID.
 *
 * @version 1.0
 */

#include "RC522.h"
#include "HAL/ISR/ISR.h"

/*---------------------------------------------------------------------------*/
/* MFRC522 Register Map (Page 0 only; upper bits are the page, always 0)     */
/*---------------------------------------------------------------------------*/

#define REG_COMMAND         0x01
#define REG_COMIEN          0x02
#define REG_DIVIEN          0x03
#define REG_COMIRQ          0x04
#define REG_DIVIRQ          0x05
#define REG_ERROR           0x06
#define REG_STATUS1         0x07
#define REG_STATUS2         0x08
#define REG_FIFODATA        0x09
#define REG_FIFOLEVEL       0x0A
#define REG_CONTROL         0x0C
#define REG_BITFRAMING      0x0D
#define REG_COLL            0x0E
#define REG_MODE            0x11
#define REG_TXMODE          0x12
#define REG_RXMODE          0x13
#define REG_TXCONTROL       0x14
#define REG_TXASK           0x15
#define REG_TMODE           0x2A
#define REG_TPRESCALER      0x2B
#define REG_TRELOAD_H       0x2C
#define REG_TRELOAD_L       0x2D
#define REG_VERSION         0x37

/* MFRC522 commands */
#define CMD_IDLE            0x00
#define CMD_TRANSCEIVE      0x0C
#define CMD_SOFTRESET       0x0F

/* ISO14443A PICC commands */
#define PICC_REQIDL         0x26   /* REQA - REQuest idle */
#define PICC_ANTICOLL       0x93   /* SEL cascade level 1 */
#define PICC_HALT           0x50

/*---------------------------------------------------------------------------*/
/* Module State                                                              */
/*---------------------------------------------------------------------------*/

#ifndef NULL
#define NULL ((void *)0)
#endif

static RC522_Config_t rc522_cfg;

/*---------------------------------------------------------------------------*/
/* SPI Register Access                                                       */
/*---------------------------------------------------------------------------*/

static inline void ss_low(void)  { GPIO_ClrPin(rc522_cfg.ss_port, rc522_cfg.ss_pin); }
static inline void ss_high(void) { GPIO_SetPin(rc522_cfg.ss_port, rc522_cfg.ss_pin); }

/**
 * @brief  Write one byte to an MFRC522 register.
 *
 * Address byte format (per MFRC522 datasheet §8.1.2.3):
 *   bit 7       : 0 = write, 1 = read
 *   bits 6..1   : register address
 *   bit 0       : reserved, must be 0
 */
static void reg_write(uint8_t reg, uint8_t val) {
    ss_low();
    (void)SPI_TransmitReceive8(rc522_cfg.spi, (uint8_t)((reg << 1) & 0x7E));
    (void)SPI_TransmitReceive8(rc522_cfg.spi, val);
    ss_high();
}

/**
 * @brief  Read one byte from an MFRC522 register.
 */
static uint8_t reg_read(uint8_t reg) {
    ss_low();
    (void)SPI_TransmitReceive8(rc522_cfg.spi,
                               (uint8_t)(0x80 | ((reg << 1) & 0x7E)));
    uint8_t v = SPI_TransmitReceive8(rc522_cfg.spi, 0x00);
    ss_high();
    return v;
}

/**
 * @brief  Clear a bit-mask in a register
 */
static void reg_clear_bits(uint8_t reg, uint8_t mask) {
    reg_write(reg, (uint8_t)(reg_read(reg) & (uint8_t)~mask));
}

/**
 * @brief  Set a bit-mask in a register
 */
static void reg_set_bits(uint8_t reg, uint8_t mask) {
    reg_write(reg, (uint8_t)(reg_read(reg) | mask));
}

/*---------------------------------------------------------------------------*/
/* Transceive Helper                                                         */
/*---------------------------------------------------------------------------*/

/**
 * @brief  Send @p tx_len bytes to a PICC and collect the reply
 *
 * Uses the Transceive command with the internal timer for card-response
 * timeout.  On entry BitFramingReg controls TX framing (7 bits for REQA,
 * 0 for full-byte commands) and must be set by the caller.
 *
 * @param[in]     tx       Bytes to transmit (copied into the FIFO)
 * @param[in]     tx_len   Number of bytes to transmit
 * @param[out]    rx       Buffer for received bytes
 * @param[in,out] rx_len   In: capacity of @p rx, Out: bytes received
 * @return  0 on success, -1 on timeout / PCD error / PICC error
 */
static int8_t transceive(const uint8_t *tx, uint8_t tx_len,
                         uint8_t *rx, uint8_t *rx_len) {
    /* Interrupt mask:
     *   bit 5 RxIRq, bit 4 IdleIRq, bit 0 TimerIRq
     * Wait pattern: RxIRq | IdleIRq. */
    const uint8_t wait_irq = 0x30;

    /* Prepare the transceiver. */
    reg_write(REG_COMMAND, CMD_IDLE);           /* stop any ongoing command */
    reg_write(REG_COMIRQ, 0x7F);                /* clear all IRQ flags */
    reg_write(REG_FIFOLEVEL, 0x80);             /* flush FIFO */

    /* Load TX payload into FIFO. */
    for (uint8_t i = 0; i < tx_len; i++) {
        reg_write(REG_FIFODATA, tx[i]);
    }

    /* Kick off the transceive. */
    reg_write(REG_COMMAND, CMD_TRANSCEIVE);
    reg_set_bits(REG_BITFRAMING, 0x80);         /* StartSend */

    /* Poll COMIRQ for completion or timer expiry.  The internal
     * timer (~25 ms) guarantees we don't hang if no card responds. */
    uint16_t guard = 2000;
    uint8_t  irq;
    do {
        irq = reg_read(REG_COMIRQ);
        if (irq & wait_irq) break;              /* data received / idle */
        if (irq & 0x01)     { guard = 0; break; }  /* timer expired */
    } while (--guard);

    /* Stop StartSend so the chip returns to idle. */
    reg_clear_bits(REG_BITFRAMING, 0x80);

    if (guard == 0) {
        if (rx_len) *rx_len = 0;
        return -1;
    }

    /* Check RX error conditions:
     *   0x13 mask = BufferOvflErr | CollErr | ParityErr | ProtocolErr */
    if (reg_read(REG_ERROR) & 0x13) {
        if (rx_len) *rx_len = 0;
        return -1;
    }

    /* Drain FIFO. */
    uint8_t n = reg_read(REG_FIFOLEVEL);
    if (rx_len) {
        if (n > *rx_len) n = *rx_len;
        for (uint8_t i = 0; i < n; i++) {
            rx[i] = reg_read(REG_FIFODATA);
        }
        *rx_len = n;
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
/* Public API                                                                */
/*---------------------------------------------------------------------------*/

void RC522_AntennaOn(void) {
    uint8_t v = reg_read(REG_TXCONTROL);
    if ((v & 0x03) != 0x03) {
        reg_write(REG_TXCONTROL, (uint8_t)(v | 0x03));
    }
}

void RC522_AntennaOff(void) {
    reg_clear_bits(REG_TXCONTROL, 0x03);
}

uint8_t RC522_GetVersion(void) {
    return reg_read(REG_VERSION);
}

int8_t RC522_Init(const RC522_Config_t *cfg) {
    if (cfg == NULL) return -1;
    rc522_cfg = *cfg;

    /* Most RC522 Arduino breakouts spec 4 MHz max.  With STM32F103 APB2
     * at 8 MHz, PCLK/4 = 2 MHz keeps us well inside spec. */
    SPI_Init(rc522_cfg.spi, SPI_PRESCALER_4, SPI_MODE_0);

    GPIO_Init(rc522_cfg.ss_port, rc522_cfg.ss_pin);
    ss_high();

    if (rc522_cfg.has_rst) {
        GPIO_Init(rc522_cfg.rst_port, rc522_cfg.rst_pin);
        GPIO_ClrPin(rc522_cfg.rst_port, rc522_cfg.rst_pin);
        SysTick_DelayMs(10);
        GPIO_SetPin(rc522_cfg.rst_port, rc522_cfg.rst_pin);
    }
    SysTick_DelayMs(50);

    /* Soft reset - clears all internal state. */
    reg_write(REG_COMMAND, CMD_SOFTRESET);
    SysTick_DelayMs(50);
    /* Wait for the PowerDown flag in CommandReg to clear. */
    uint16_t guard = 1000;
    while ((reg_read(REG_COMMAND) & 0x10) && --guard) {
        SysTick_DelayMs(1);
    }

    /* Internal timer for card-response timeout (~25 ms):
     *   fTimer = 13.56 MHz / (2 * TPrescaler + 1) ≈ 40 kHz with prescaler 0xA9
     *   TReload = 1000 ticks -> 25 ms */
    reg_write(REG_TMODE,      0x80);   /* auto-start, TPrescaler hi = 0 */
    reg_write(REG_TPRESCALER, 0xA9);
    reg_write(REG_TRELOAD_H,  0x03);
    reg_write(REG_TRELOAD_L,  0xE8);

    /* Force 100% ASK modulation (required for ISO14443A). */
    reg_write(REG_TXASK, 0x40);

    /* TX/RX mode: CRC preset 6363h, 106 kbps. */
    reg_write(REG_MODE, 0x3D);

    /* Turn on the antenna. */
    RC522_AntennaOn();

    /* Sanity check the chip version. */
    uint8_t v = reg_read(REG_VERSION);
    if (v != RC522_VERSION_1 &&
        v != RC522_VERSION_2 &&
        v != RC522_VERSION_CLONE) {
        return -1;
    }
    return 0;
}

int8_t RC522_IsCardPresent(void) {
    /* REQA uses a 7-bit short frame. */
    reg_write(REG_BITFRAMING, 0x07);

    uint8_t tx  = PICC_REQIDL;
    uint8_t rx[8];
    uint8_t rx_len = sizeof(rx);
    if (transceive(&tx, 1, rx, &rx_len) != 0) {
        return -1;
    }
    /* A valid ATQA reply is exactly 2 bytes (16 bits). */
    return (rx_len == 2) ? 0 : -1;
}

int8_t RC522_ReadUID(uint8_t uid[RC522_UID_LEN]) {
    if (uid == NULL) return -1;

    /* Step 1 - REQA (wakes up cards in the IDLE state). */
    reg_write(REG_BITFRAMING, 0x07);
    {
        uint8_t tx = PICC_REQIDL;
        uint8_t rx[2];
        uint8_t rx_len = sizeof(rx);
        if (transceive(&tx, 1, rx, &rx_len) != 0 || rx_len != 2) {
            return -1;
        }
    }

    /* Step 2 - Anticollision cascade level 1 (get 4 UID bytes + BCC). */
    reg_write(REG_BITFRAMING, 0x00);   /* full-byte frames */
    reg_write(REG_COLL, 0x80);         /* clear any stale collision state */
    {
        uint8_t tx[2] = { PICC_ANTICOLL, 0x20 };  /* NVB = 0x20, no known bits */
        uint8_t rx[5];
        uint8_t rx_len = sizeof(rx);
        if (transceive(tx, 2, rx, &rx_len) != 0 || rx_len != 5) {
            return -1;
        }
        /* Verify BCC = UID[0] ^ UID[1] ^ UID[2] ^ UID[3]. */
        uint8_t bcc = (uint8_t)(rx[0] ^ rx[1] ^ rx[2] ^ rx[3]);
        if (bcc != rx[4]) return -1;

        uid[0] = rx[0];
        uid[1] = rx[1];
        uid[2] = rx[2];
        uid[3] = rx[3];
    }
    return 0;
}

void RC522_Halt(void) {
    /* HALT_A: 0x50 0x00 + CRC_A.  We skip the CRC - the card ignores a
     * malformed HALT and we achieve the same "don't re-respond to REQA"
     * effect by leaving the field and re-doing REQA on the next scan. */
    reg_write(REG_BITFRAMING, 0x00);
    uint8_t tx[2] = { PICC_HALT, 0x00 };
    uint8_t rx[2];
    uint8_t rx_len = sizeof(rx);
    (void)transceive(tx, 2, rx, &rx_len);
}

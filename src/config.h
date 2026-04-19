/**
 * @file    config.h
 * @brief   Hardware pin configuration for BluePill OS
 */

#ifndef CONFIG_H
#define CONFIG_H

/* Buzzer */
#define BUZZ_PORT   GPIO_PORT_B
#define BUZZ_PIN    GPIO_PIN_4

/* Relays */
#define REL1_PORT   GPIO_PORT_B
#define REL1_PIN    GPIO_PIN_5
#define REL2_PORT   GPIO_PORT_A
#define REL2_PIN    GPIO_PIN_3

/* LEDs */
#define RED1_PORT   GPIO_PORT_A
#define RED1_PIN    GPIO_PIN_2
#define RED2_PORT   GPIO_PORT_A
#define RED2_PIN    GPIO_PIN_1
#define GRN_PORT    GPIO_PORT_C
#define GRN_PIN     GPIO_PIN_15
#define BLU_PORT    GPIO_PORT_C
#define BLU_PIN     GPIO_PIN_14
#define YLW_PORT    GPIO_PORT_C
#define YLW_PIN     GPIO_PIN_13

/* ---- 16x2 LCD Pin Config ---- */
static LCD_Pins_t lcd_pins = {
    .rs_port = GPIO_PORT_A, .rs_pin = GPIO_PIN_11,
    .en_port = GPIO_PORT_A, .en_pin = GPIO_PIN_8,
    .d4_port = GPIO_PORT_B, .d4_pin = GPIO_PIN_15,
    .d5_port = GPIO_PORT_B, .d5_pin = GPIO_PIN_14,
    .d6_port = GPIO_PORT_B, .d6_pin = GPIO_PIN_13,
    .d7_port = GPIO_PORT_B, .d7_pin = GPIO_PIN_12,
};

/* ---- 4x4 Keypad Pin Config ---- */
static Keypad_Config_t keypad_cfg = {
    .rows = {
        { GPIO_PORT_A, GPIO_PIN_4 },
        { GPIO_PORT_A, GPIO_PIN_5 },
        { GPIO_PORT_A, GPIO_PIN_6 },
        { GPIO_PORT_A, GPIO_PIN_7 },
    },
    .cols = {
        { GPIO_PORT_B, GPIO_PIN_11 },
        { GPIO_PORT_B, GPIO_PIN_10 },
        { GPIO_PORT_B, GPIO_PIN_1  },
        { GPIO_PORT_B, GPIO_PIN_0  },
    },
    .num_rows = 4,
    .num_cols = 4,
    .keymap = {
        { '1', '2', '3', 'A' },
        { '4', '5', '6', 'B' },
        { '7', '8', '9', 'C' },
        { '*', '0', '#', 'D' },
    },
};

// /* ---- 3-Digit 7-Segment (multiplexed) ---- */
// static SEG7_Config_t seg7_cfg = {
//     .seg = {
//         { GPIO_PORT_A, GPIO_PIN_4  },  /* a   */
//         { GPIO_PORT_A, GPIO_PIN_8  },  /* b   */
//         { GPIO_PORT_B, GPIO_PIN_15 },  /* c   */
//         { GPIO_PORT_B, GPIO_PIN_14 },  /* d   */
//         { GPIO_PORT_B, GPIO_PIN_13 },  /* e   */
//         { GPIO_PORT_B, GPIO_PIN_12 },  /* f   */
//         { GPIO_PORT_A, GPIO_PIN_12 },  /* g   */
//         { GPIO_PORT_A, GPIO_PIN_15 },  /* dp  */
//     },
//     .dig = {
//         { GPIO_PORT_B, GPIO_PIN_8 },   /* COM1 (rightmost) */
//         { GPIO_PORT_B, GPIO_PIN_9 },   /* COM2 (middle)    */
//         { GPIO_PORT_B, GPIO_PIN_3 },   /* COM3 (leftmost)  */
//     },
//     .num_digits = 3,
//     .common_anode = 1,
// };

/* ---- RFID-RC522 (MFRC522) via SPI2 ----
 *
 * Pins:  SCK=PB13  MISO=PB14  MOSI=PB15  SS=PB9  RST=PB8
 *
 * PIN CONFLICT: PB13/14/15 are also LCD D6/D5/D4.  When SPI2 is
 * enabled those pins are driven by the SPI peripheral as AF push-pull,
 * so the 16x2 LCD displays garbage until the RFID app exits and
 * re-initialises the LCD (LCD_Init(&lcd_pins)).  The RFID app uses
 * the OLED for all feedback while active.
 */
static RC522_Config_t rc522_cfg = {
    .spi      = SPI_2,
    .ss_port  = GPIO_PORT_B, .ss_pin  = GPIO_PIN_9,
    .rst_port = GPIO_PORT_B, .rst_pin = GPIO_PIN_8,
    .has_rst  = 1,
};

#endif /* CONFIG_H */

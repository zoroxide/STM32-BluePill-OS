/**
 * @file    LCD.h
 * @brief   16x2 Character LCD (HD44780) Driver
 *
 * Provides a simple interface for HD44780-compatible 16x2 character LCDs
 * using 4-bit mode. All pins are configurable via the LCD_Pins_t struct.
 *
 * @note    Uses GPIO HAL for pin control. Operates in 4-bit mode to save pins.
 *          Requires 6 GPIO pins: RS, EN, D4, D5, D6, D7
 *
 * @version 1.0
 */

#ifndef DRIVER_LCD_H
#define DRIVER_LCD_H

#include <stdint.h>
#include "HAL/IO/IO.h"

#define LCD_COLS    16
#define LCD_ROWS    2

#define LCD_CMD_CLEAR           0x01
#define LCD_CMD_HOME            0x02
#define LCD_CMD_ENTRY_MODE      0x06
#define LCD_CMD_DISPLAY_ON      0x0C
#define LCD_CMD_DISPLAY_OFF     0x08
#define LCD_CMD_CURSOR_ON       0x0E
#define LCD_CMD_CURSOR_BLINK    0x0F
#define LCD_CMD_FUNCTION_4BIT   0x28
#define LCD_CMD_SET_DDRAM       0x80
#define LCD_CMD_SET_CGRAM       0x40

/**
 * @struct LCD_Pins_t
 * @brief  Pin configuration for the LCD
 */
typedef struct {
    GPIO_Port_t rs_port;    GPIO_Pin_t rs_pin;
    GPIO_Port_t en_port;    GPIO_Pin_t en_pin;
    GPIO_Port_t d4_port;    GPIO_Pin_t d4_pin;
    GPIO_Port_t d5_port;    GPIO_Pin_t d5_pin;
    GPIO_Port_t d6_port;    GPIO_Pin_t d6_pin;
    GPIO_Port_t d7_port;    GPIO_Pin_t d7_pin;
} LCD_Pins_t;

void LCD_Init(const LCD_Pins_t *pins);
void LCD_Clear(void);
void LCD_Home(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_PrintChar(char c);
void LCD_PrintString(const char *str);
void LCD_PrintInt(int32_t num);
void LCD_Command(uint8_t cmd);
void LCD_CreateChar(uint8_t location, const uint8_t *pattern);

#endif /* DRIVER_LCD_H */

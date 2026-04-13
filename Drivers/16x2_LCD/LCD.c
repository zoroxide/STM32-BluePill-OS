/**
 * @file    LCD.c
 * @brief   16x2 Character LCD (HD44780) Driver Implementation
 */

#include "LCD.h"

static const uint8_t LCD_ROW_ADDR[LCD_ROWS] = { 0x00, 0x40 };
static LCD_Pins_t lcd_pins;

static void LCD_DelayShort(void) {
    volatile uint32_t i;
    for (i = 0; i < 10; i++) {}
}

static void LCD_DelayLong(void) {
    volatile uint32_t i;
    for (i = 0; i < 5000; i++) {}
}

static void LCD_PulseEnable(void) {
    GPIO_SetPin(lcd_pins.en_port, lcd_pins.en_pin);
    LCD_DelayShort();
    GPIO_ClrPin(lcd_pins.en_port, lcd_pins.en_pin);
    LCD_DelayShort();
}

static void LCD_WriteNibble(uint8_t nibble) {
    if (nibble & 0x01) { GPIO_SetPin(lcd_pins.d4_port, lcd_pins.d4_pin); }
    else               { GPIO_ClrPin(lcd_pins.d4_port, lcd_pins.d4_pin); }
    if (nibble & 0x02) { GPIO_SetPin(lcd_pins.d5_port, lcd_pins.d5_pin); }
    else               { GPIO_ClrPin(lcd_pins.d5_port, lcd_pins.d5_pin); }
    if (nibble & 0x04) { GPIO_SetPin(lcd_pins.d6_port, lcd_pins.d6_pin); }
    else               { GPIO_ClrPin(lcd_pins.d6_port, lcd_pins.d6_pin); }
    if (nibble & 0x08) { GPIO_SetPin(lcd_pins.d7_port, lcd_pins.d7_pin); }
    else               { GPIO_ClrPin(lcd_pins.d7_port, lcd_pins.d7_pin); }
    LCD_PulseEnable();
}

static void LCD_WriteByte(uint8_t rs, uint8_t byte) {
    if (rs) { GPIO_SetPin(lcd_pins.rs_port, lcd_pins.rs_pin); }
    else    { GPIO_ClrPin(lcd_pins.rs_port, lcd_pins.rs_pin); }
    LCD_WriteNibble((byte >> 4) & 0x0F);
    LCD_WriteNibble(byte & 0x0F);
}

void LCD_Init(const LCD_Pins_t *pins) {
    volatile uint32_t i;
    lcd_pins = *pins;

    GPIO_Init(pins->rs_port, pins->rs_pin);
    GPIO_Init(pins->en_port, pins->en_pin);
    GPIO_Init(pins->d4_port, pins->d4_pin);
    GPIO_Init(pins->d5_port, pins->d5_pin);
    GPIO_Init(pins->d6_port, pins->d6_pin);
    GPIO_Init(pins->d7_port, pins->d7_pin);

    for (i = 0; i < 100000; i++) {}

    GPIO_ClrPin(lcd_pins.rs_port, lcd_pins.rs_pin);
    LCD_WriteNibble(0x03); LCD_DelayLong();
    LCD_WriteNibble(0x03); LCD_DelayLong();
    LCD_WriteNibble(0x03); LCD_DelayLong();
    LCD_WriteNibble(0x02); LCD_DelayLong();

    LCD_Command(LCD_CMD_FUNCTION_4BIT);
    LCD_Command(LCD_CMD_DISPLAY_ON);
    LCD_Command(LCD_CMD_ENTRY_MODE);
    LCD_Clear();
}

void LCD_Clear(void) {
    LCD_WriteByte(0, LCD_CMD_CLEAR);
    LCD_DelayLong();
}

void LCD_Home(void) {
    LCD_WriteByte(0, LCD_CMD_HOME);
    LCD_DelayLong();
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    if (row >= LCD_ROWS) { row = 0; }
    if (col >= LCD_COLS) { col = 0; }
    LCD_WriteByte(0, LCD_CMD_SET_DDRAM | (LCD_ROW_ADDR[row] + col));
}

void LCD_PrintChar(char c) { LCD_WriteByte(1, (uint8_t)c); }

void LCD_PrintString(const char *str) {
    while (*str) { LCD_PrintChar(*str); str++; }
}

void LCD_PrintInt(int32_t num) {
    char buf[12];
    char *p = buf + sizeof(buf) - 1;
    uint32_t uval;
    uint8_t negative = 0;
    *p = '\0';
    if (num < 0) { negative = 1; uval = (uint32_t)(-(num + 1)) + 1; }
    else { uval = (uint32_t)num; }
    if (uval == 0) { *(--p) = '0'; }
    else { while (uval > 0) { *(--p) = '0' + (char)(uval % 10); uval /= 10; } }
    if (negative) { *(--p) = '-'; }
    LCD_PrintString(p);
}

void LCD_Command(uint8_t cmd) {
    LCD_WriteByte(0, cmd);
    LCD_DelayShort();
}

void LCD_CreateChar(uint8_t location, const uint8_t *pattern) {
    uint8_t i;
    location &= 0x07;
    LCD_WriteByte(0, LCD_CMD_SET_CGRAM | (location << 3));
    for (i = 0; i < 8; i++) { LCD_WriteByte(1, pattern[i]); }
}

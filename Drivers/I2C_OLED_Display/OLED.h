/**
 * @file    OLED.h
 * @brief   SSD1306 I2C OLED Display Driver (128x64)
 *
 * Framebuffer-based driver for SSD1306 OLED displays over I2C.
 * All drawing happens in RAM; call OLED_Update() to flush to display.
 *
 * @note    Typical I2C address: 0x3C (or 0x3D if SA0 is HIGH).
 *          Uses 1024 bytes of RAM for the 128x64 framebuffer.
 * @version 1.0
 */

#ifndef DRIVER_OLED_H
#define DRIVER_OLED_H

#include <stdint.h>
#include "HAL/I2C/I2C.h"

#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_PAGES      (OLED_HEIGHT / 8)
#define OLED_BUF_SIZE   (OLED_WIDTH * OLED_PAGES)

#define OLED_WHITE      1
#define OLED_BLACK      0

void OLED_Init(I2C_Peripheral_t i2c, uint8_t addr);
void OLED_Clear(void);
void OLED_Fill(void);
void OLED_Update(void);
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void OLED_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void OLED_SetCursor(uint8_t x, uint8_t y);
void OLED_PrintChar(char c);
void OLED_PrintString(const char *str);
void OLED_PrintInt(int32_t num);
void OLED_DrawCircle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t color);

#endif /* DRIVER_OLED_H */

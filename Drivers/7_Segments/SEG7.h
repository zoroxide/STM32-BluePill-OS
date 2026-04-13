/**
 * @file    SEG7.h
 * @brief   7-Segment Display Driver
 *
 * Supports single or multi-digit (up to 4) 7-segment displays with
 * software multiplexing. Works with both common anode and common cathode.
 *
 * Segment layout:    a
 *                   ---
 *                f |   | b
 *                   -g-
 *                e |   | c
 *                   ---  . dp
 *                    d
 *
 * @note    For multi-digit displays, call SEG7_Refresh() periodically
 *          (every ~5ms) for flicker-free multiplexing.
 * @version 1.0
 */

#ifndef DRIVER_SEG7_H
#define DRIVER_SEG7_H

#include <stdint.h>
#include "HAL/IO/IO.h"

#define SEG7_MAX_DIGITS     4

typedef struct {
    struct { GPIO_Port_t port; GPIO_Pin_t pin; } seg[8];
    struct { GPIO_Port_t port; GPIO_Pin_t pin; } dig[SEG7_MAX_DIGITS];
    uint8_t num_digits;
    uint8_t common_anode;
} SEG7_Config_t;

void SEG7_Init(const SEG7_Config_t *config);
void SEG7_DisplayDigit(uint8_t position, uint8_t digit, uint8_t dp);
void SEG7_DisplayNumber(int16_t number);
void SEG7_Clear(void);
void SEG7_Refresh(void);
void SEG7_DisplayRaw(uint8_t position, uint8_t pattern);

#endif /* DRIVER_SEG7_H */

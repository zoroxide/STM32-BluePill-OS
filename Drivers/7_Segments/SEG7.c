/**
 * @file    SEG7.c
 * @brief   7-Segment Display Driver Implementation
 */

#include "SEG7.h"

/** @brief Segment patterns for digits 0-9 (bits: dp g f e d c b a) */
static const uint8_t SEG7_DIGITS[10] = {
    0x3F, /* 0 */ 0x06, /* 1 */ 0x5B, /* 2 */ 0x4F, /* 3 */ 0x66, /* 4 */
    0x6D, /* 5 */ 0x7D, /* 6 */ 0x07, /* 7 */ 0x7F, /* 8 */ 0x6F, /* 9 */
};

static SEG7_Config_t seg7_cfg;
static uint8_t seg7_buffer[SEG7_MAX_DIGITS];
static uint8_t seg7_current_digit;

static void SEG7_AllDigitsOff(void) {
    uint8_t i;
    for (i = 0; i < seg7_cfg.num_digits; i++) {
        if (seg7_cfg.common_anode)
            GPIO_SetPin(seg7_cfg.dig[i].port, seg7_cfg.dig[i].pin);
        else
            GPIO_ClrPin(seg7_cfg.dig[i].port, seg7_cfg.dig[i].pin);
    }
}

static void SEG7_SetSegments(uint8_t pattern) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        uint8_t on = (pattern >> i) & 0x01;
        if (seg7_cfg.common_anode) { on = !on; }
        if (on) { GPIO_SetPin(seg7_cfg.seg[i].port, seg7_cfg.seg[i].pin); }
        else    { GPIO_ClrPin(seg7_cfg.seg[i].port, seg7_cfg.seg[i].pin); }
    }
}

static void SEG7_EnableDigit(uint8_t pos) {
    if (seg7_cfg.common_anode)
        GPIO_ClrPin(seg7_cfg.dig[pos].port, seg7_cfg.dig[pos].pin);
    else
        GPIO_SetPin(seg7_cfg.dig[pos].port, seg7_cfg.dig[pos].pin);
}

void SEG7_Init(const SEG7_Config_t *config) {
    uint8_t i;
    seg7_cfg = *config;
    seg7_current_digit = 0;
    for (i = 0; i < 8; i++)
        GPIO_Init(config->seg[i].port, config->seg[i].pin);
    for (i = 0; i < config->num_digits; i++)
        GPIO_Init(config->dig[i].port, config->dig[i].pin);
    SEG7_Clear();
}

void SEG7_DisplayDigit(uint8_t position, uint8_t digit, uint8_t dp) {
    if (position >= seg7_cfg.num_digits) { return; }
    if (digit > 9) { seg7_buffer[position] = 0x00; }
    else { seg7_buffer[position] = SEG7_DIGITS[digit] | (dp ? 0x80 : 0x00); }
}

void SEG7_DisplayNumber(int16_t number) {
    uint8_t i, neg = 0;
    uint16_t val;
    if (number < 0) { neg = 1; val = (uint16_t)(-number); }
    else { val = (uint16_t)number; }
    for (i = 0; i < seg7_cfg.num_digits; i++) { seg7_buffer[i] = 0x00; }
    for (i = 0; i < seg7_cfg.num_digits; i++) {
        seg7_buffer[i] = SEG7_DIGITS[val % 10];
        val /= 10;
        if (val == 0) { break; }
    }
    if (neg && (i + 1) < seg7_cfg.num_digits) { seg7_buffer[i + 1] = 0x40; }
}

void SEG7_Clear(void) {
    uint8_t i;
    for (i = 0; i < SEG7_MAX_DIGITS; i++) { seg7_buffer[i] = 0x00; }
    SEG7_AllDigitsOff();
    SEG7_SetSegments(0x00);
}

void SEG7_Refresh(void) {
    SEG7_AllDigitsOff();
    SEG7_SetSegments(seg7_buffer[seg7_current_digit]);
    SEG7_EnableDigit(seg7_current_digit);
    seg7_current_digit++;
    if (seg7_current_digit >= seg7_cfg.num_digits) { seg7_current_digit = 0; }
}

void SEG7_DisplayRaw(uint8_t position, uint8_t pattern) {
    if (position >= seg7_cfg.num_digits) { return; }
    seg7_buffer[position] = pattern;
}

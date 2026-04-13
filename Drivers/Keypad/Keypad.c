/**
 * @file    Keypad.c
 * @brief   Matrix Keypad Driver Implementation
 */

#include "Keypad.h"

static Keypad_Config_t kp_cfg;
#define KEYPAD_DEBOUNCE_DELAY   5000

static void Keypad_Delay(void) {
    volatile uint32_t i;
    for (i = 0; i < KEYPAD_DEBOUNCE_DELAY; i++) {}
}

static void Keypad_AllRowsHigh(void) {
    uint8_t r;
    for (r = 0; r < kp_cfg.num_rows; r++)
        GPIO_SetPin(kp_cfg.rows[r].port, kp_cfg.rows[r].pin);
}

void Keypad_Init(const Keypad_Config_t *config) {
    uint8_t i;
    kp_cfg = *config;
    for (i = 0; i < config->num_rows; i++) {
        GPIO_Init(config->rows[i].port, config->rows[i].pin);
        GPIO_SetPin(config->rows[i].port, config->rows[i].pin);
    }
    for (i = 0; i < config->num_cols; i++) {
        GPIO_InitInput(config->cols[i].port, config->cols[i].pin);
    }
}

char Keypad_Scan(void) {
    uint8_t r, c;
    for (r = 0; r < kp_cfg.num_rows; r++) {
        Keypad_AllRowsHigh();
        GPIO_ClrPin(kp_cfg.rows[r].port, kp_cfg.rows[r].pin);
        volatile uint32_t s; for (s = 0; s < 100; s++) {}
        for (c = 0; c < kp_cfg.num_cols; c++) {
            if (GPIO_ReadPin(kp_cfg.cols[c].port, kp_cfg.cols[c].pin) == 0) {
                Keypad_Delay();
                if (GPIO_ReadPin(kp_cfg.cols[c].port, kp_cfg.cols[c].pin) == 0) {
                    Keypad_AllRowsHigh();
                    return kp_cfg.keymap[r][c];
                }
            }
        }
    }
    Keypad_AllRowsHigh();
    return KEYPAD_NO_KEY;
}

char Keypad_GetKey(void) {
    char key;
    do { key = Keypad_Scan(); } while (key == KEYPAD_NO_KEY);
    while (Keypad_Scan() != KEYPAD_NO_KEY) {}
    Keypad_Delay();
    return key;
}

uint8_t Keypad_IsPressed(void) {
    return (Keypad_Scan() != KEYPAD_NO_KEY) ? 1 : 0;
}

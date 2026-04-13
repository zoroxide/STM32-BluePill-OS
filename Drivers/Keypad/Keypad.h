/**
 * @file    Keypad.h
 * @brief   Matrix Keypad Driver (up to 4x4)
 *
 * Supports 3x3, 3x4, 4x4, or custom matrix keypads.
 * Rows are outputs, columns are inputs with pull-ups.
 *
 * Default 4x4 key map:
 *   C0  C1  C2  C3
 *   1   2   3   A    R0
 *   4   5   6   B    R1
 *   7   8   9   C    R2
 *   *   0   #   D    R3
 *
 * @note    Includes software debouncing.
 * @version 1.0
 */

#ifndef DRIVER_KEYPAD_H
#define DRIVER_KEYPAD_H

#include <stdint.h>
#include "HAL/IO/IO.h"

#define KEYPAD_MAX_ROWS     4
#define KEYPAD_MAX_COLS     4
#define KEYPAD_NO_KEY       '\0'

typedef struct {
    struct { GPIO_Port_t port; GPIO_Pin_t pin; } rows[KEYPAD_MAX_ROWS];
    struct { GPIO_Port_t port; GPIO_Pin_t pin; } cols[KEYPAD_MAX_COLS];
    uint8_t num_rows;
    uint8_t num_cols;
    char keymap[KEYPAD_MAX_ROWS][KEYPAD_MAX_COLS];
} Keypad_Config_t;

void Keypad_Init(const Keypad_Config_t *config);
char Keypad_Scan(void);
char Keypad_GetKey(void);
uint8_t Keypad_IsPressed(void);

#endif /* DRIVER_KEYPAD_H */

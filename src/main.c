/**
 * @file main.c
 * @brief BluePill OS - Simple embedded OS for STM32F103C8T6
 */

#include "../Drivers/16x2_LCD/LCD.h"
#include "../Drivers/I2C_OLED_Display/OLED.h"
#include "../Drivers/Keypad/Keypad.h"
#include "../Drivers/7_Segments/SEG7.h"
#include "../Drivers/PN532_NFC/PN532.h"
#include "../HAL/ADC/ADC.h"
#include "../HAL/ISR/ISR.h"
#include "utils/delay.h"
#include "utils/math.c"
#include "config.h"

int16_t frame = 0;

#include "os.c"

/*---------------------------------------------------------------------------*/
/* Linux-style boot log on OLED                                              */
/*---------------------------------------------------------------------------*/

static uint8_t boot_line = 0;

static void boot_log(const char *msg) {
    uint8_t y = boot_line * 8;
    if (y + 8 > 64) {
        /* Scroll: clear and reset to line 6 to keep last lines visible */
        OLED_Clear();
        boot_line = 0;
        y = 0;
    }
    OLED_SetCursor(0, y);
    OLED_PrintString("[ ** ] ");
    OLED_PrintString(msg);
    OLED_Update();
    SysTick_DelayMs(120);

    /* Replace ** with OK */
    OLED_FillRect(0, y, 42, 8, OLED_BLACK);
    OLED_SetCursor(0, y);
    OLED_PrintString("[ OK ] ");
    OLED_PrintString(msg);
    OLED_Update();
    SysTick_DelayMs(80);

    boot_line++;
}

int main(void) {

    /* ---- Early init (needed for boot log timing) ---- */
    SysTick_Init(1000);
    ISR_GlobalEnable();

    /* ---- OLED first (we need it for the boot log) ---- */
    I2C_Init(I2C_1, I2C_SPEED_400K);
    OLED_Init(I2C_1, 0x3C);

    /* ---- Boot log header ---- */
    OLED_Clear();
    OLED_SetCursor(10, 0);
    OLED_PrintString("BluePill OS v1.0");
    OLED_DrawLine(0, 9, 127, 9, OLED_WHITE);
    OLED_Update();
    SysTick_DelayMs(300);
    boot_line = 2;

    /* ---- Init peripherals with boot log ---- */
    boot_log("SysTick 1kHz");

    boot_log("I2C1 Bus");

    boot_log("OLED 128x64");

    ADC_Init(ADC_1);
    boot_log("ADC1 12-bit");

    LCD_Init(&lcd_pins);
    boot_log("LCD 16x2");

    /* Show on LCD too now that it's ready */
    LCD_SetCursor(0, 0);
    LCD_PrintString("BluePill OS v1.0");
    LCD_SetCursor(1, 0);
    LCD_PrintString("Booting...      ");

    Keypad_Init(&keypad_cfg);
    boot_log("Keypad 4x4");

    GPIO_Init(BUZZ_PORT, BUZZ_PIN);
    GPIO_ClrPin(BUZZ_PORT, BUZZ_PIN);
    boot_log("Buzzer PB4");

    /* Quick beep to confirm buzzer works */
    GPIO_SetPin(BUZZ_PORT, BUZZ_PIN);
    delay(50000);
    GPIO_ClrPin(BUZZ_PORT, BUZZ_PIN);

    GPIO_Init(REL1_PORT, REL1_PIN);
    GPIO_Init(REL2_PORT, REL2_PIN);
    GPIO_ClrPin(REL1_PORT, REL1_PIN);
    GPIO_ClrPin(REL2_PORT, REL2_PIN);

    /* Scroll to next page */
    OLED_Clear();
    boot_line = 0;
    boot_log("Relay 1 PB5");
    boot_log("Relay 2 PA3");

    GPIO_Init(RED1_PORT, RED1_PIN);
    GPIO_Init(RED2_PORT, RED2_PIN);
    GPIO_Init(GRN_PORT,  GRN_PIN);
    GPIO_Init(BLU_PORT,  BLU_PIN);
    GPIO_Init(YLW_PORT,  YLW_PIN);
    GPIO_ClrPin(RED1_PORT, RED1_PIN);
    GPIO_ClrPin(RED2_PORT, RED2_PIN);
    GPIO_ClrPin(GRN_PORT,  GRN_PIN);
    GPIO_ClrPin(BLU_PORT,  BLU_PIN);
    GPIO_ClrPin(YLW_PORT,  YLW_PIN);
    boot_log("LEDs x5");

    /* Flash all LEDs briefly to confirm */
    GPIO_SetPin(RED1_PORT, RED1_PIN);
    GPIO_SetPin(RED2_PORT, RED2_PIN);
    GPIO_SetPin(GRN_PORT,  GRN_PIN);
    GPIO_SetPin(BLU_PORT,  BLU_PIN);
    GPIO_SetPin(YLW_PORT,  YLW_PIN);
    SysTick_DelayMs(150);
    GPIO_ClrPin(RED1_PORT, RED1_PIN);
    GPIO_ClrPin(RED2_PORT, RED2_PIN);
    GPIO_ClrPin(GRN_PORT,  GRN_PIN);
    GPIO_ClrPin(BLU_PORT,  BLU_PIN);
    GPIO_ClrPin(YLW_PORT,  YLW_PIN);

    // SEG7_Init(&seg7_cfg);
    // SysTick_SetCallback(SEG7_Refresh);
    // boot_log("7-Seg x3");

    boot_log("OS Kernel");

    /* Final boot message */
    uint8_t y = boot_line * 8;
    OLED_SetCursor(0, y + 4);
    OLED_PrintString("All systems GO!");
    OLED_Update();

    LCD_SetCursor(1, 0);
    LCD_PrintString("Ready!          ");

    SysTick_DelayMs(600);

    /* ---- Boot into OS (never returns) ---- */
    os_run();

    return 0;
}

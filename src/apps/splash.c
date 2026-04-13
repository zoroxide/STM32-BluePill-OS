
static void app_splash(void) {
    LCD_Clear();
    os_lcd_print_centered(0, "BluePill OS");
    os_lcd_print_centered(1, "Press any key..");

    OLED_Clear();

    OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
    OLED_DrawRect(2, 2, 124, 60, OLED_WHITE);

    OLED_SetCursor(16, 12);
    OLED_PrintString("BluePill OS");
    OLED_SetCursor(28, 26);
    OLED_PrintString("v1.0");

    OLED_DrawLine(10, 38, 118, 38, OLED_WHITE);

    OLED_SetCursor(10, 44);
    OLED_PrintString("Created by");
    OLED_SetCursor(16, 54);
    OLED_PrintString("Loay & Nour");

    OLED_Update();

    uint8_t anim = 0;
    while (1) {
        /* Animate corner dots */
        uint8_t ax = 6 + (anim % 20) * 6;
        uint8_t ay = (anim & 1) ? 4 : 59;
        OLED_DrawPixel(ax, ay, OLED_WHITE);
        OLED_Update();

        anim++;
        if (anim > 40) anim = 0;

        char key = Keypad_Scan();
        if (key != KEYPAD_NO_KEY) {
            while (Keypad_Scan() != KEYPAD_NO_KEY) {}
            delay(10000);
            break;
        }
        SysTick_DelayMs(50);
    }
}

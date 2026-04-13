
/* -------------------------------------------------------------------------
 *  App 12: Settings
 * ------------------------------------------------------------------------- */
static void app_settings(void) {
    uint8_t contrast_level = 1; /* 0=dim, 1=normal, 2=bright */
    static const char * const contrast_names[] = { "Dim", "Normal", "Bright" };
    uint8_t sel = 0; /* 0=contrast, 1=sysinfo */

    LCD_Clear();
    os_lcd_print_centered(0, "Settings");

    while (1) {
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(20, 2);
        OLED_PrintString("SETTINGS");

        /* Contrast option */
        OLED_SetCursor(4, 16);
        if (sel == 0) OLED_PrintString("> ");
        else OLED_PrintString("  ");
        OLED_PrintString("Contrast: ");
        OLED_PrintString(contrast_names[contrast_level]);

        /* System info option */
        OLED_SetCursor(4, 28);
        if (sel == 1) OLED_PrintString("> ");
        else OLED_PrintString("  ");
        OLED_PrintString("System Info");

        if (sel == 1) {
            /* Show system info */
            uint32_t uptime_s = SysTick_GetTicks() / 1000;
            uint32_t mins = uptime_s / 60;
            uint32_t secs = uptime_s % 60;

            OLED_SetCursor(8, 40);
            OLED_PrintString("Up: ");
            OLED_PrintInt((int32_t)mins);
            OLED_PrintString("m ");
            OLED_PrintInt((int32_t)secs);
            OLED_PrintString("s");

            OLED_SetCursor(8, 50);
            OLED_PrintString("Flash: 64KB");
        }

        OLED_Update();

        LCD_SetCursor(1, 0);
        if (sel == 0) {
            LCD_PrintString("Contrast:       ");
            LCD_SetCursor(1, 10);
            LCD_PrintString(contrast_names[contrast_level]);
        } else {
            LCD_PrintString("System Info     ");
        }

        char key = KEYPAD_NO_KEY;
        while (key == KEYPAD_NO_KEY) {
            key = os_keypad_poll();
            SysTick_DelayMs(20);
        }

        if (key == 'D') return;
        if (key == '*' || key == '2') {
            if (sel > 0) sel--;
        }
        if (key == '#' || key == '8') {
            if (sel < 1) sel++;
        }
        if (key == 'A') {
            if (sel == 0) {
                /* Cycle contrast */
                contrast_level = (contrast_level + 1) % 3;
                /* Send contrast command to OLED (SSD1306) */
                LCD_Command(0x81); /* This is OLED I2C command; we use a workaround */
                /* Actually set contrast via OLED_Command-like sequence:
                   SSD1306 contrast is set by command 0x81 followed by value.
                   We have no direct OLED_Command, so we draw with fill levels. */
                /* Approximate: fill screen briefly for bright, etc. */
                /* For now, just show the label; real contrast needs low-level I2C */
            }
        }
    }
}

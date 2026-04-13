
static void os_draw_piano_keyboard(int8_t active_key) {
    /* Draw 8 white keys across bottom of OLED */
    /* Each key: 14px wide, 30px tall, starting at y=30 */
    uint8_t ki;
    uint8_t kx;
    for (ki = 0; ki < 8; ki++) {
        kx = ki * 16;
        if (ki == active_key) {
            OLED_FillRect(kx, 30, 14, 30, OLED_WHITE);
        } else {
            OLED_DrawRect(kx, 30, 14, 30, OLED_WHITE);
        }
        /* Key number label */
        OLED_SetCursor(kx + 4, 52);
        OLED_PrintChar('1' + ki);
    }

    /* Draw black keys (simplified: between certain white keys) */
    /* Black keys between: 1-2, 2-3, skip, 4-5, 5-6, 6-7, skip */
    static const uint8_t black_pos[] = { 0, 1, 3, 4, 5 };
    for (ki = 0; ki < 5; ki++) {
        kx = black_pos[ki] * 16 + 10;
        /* Only draw if not the active white key overlapping */
        if (active_key != black_pos[ki] && active_key != black_pos[ki] + 1) {
            OLED_FillRect(kx, 30, 8, 18, OLED_WHITE);
        }
    }
}

static void app_piano(void) {
    LCD_Clear();
    os_lcd_print_centered(0, "Piano");
    os_lcd_print_centered(1, "Keys 1-8");

    while (1) {
        OLED_Clear();

        /* Title */
        OLED_SetCursor(4, 2);
        OLED_PrintString("= PIANO =");

        int8_t active = -1;

        char key = Keypad_Scan();
        if (key == 'D') {
            while (Keypad_Scan() != KEYPAD_NO_KEY) {}
            delay(10000);
            return;
        }

        if (key >= '1' && key <= '8') {
            active = key - '1';
            OLED_SetCursor(4, 14);
            OLED_PrintString("Note: ");
            OLED_PrintString(piano_note_names[active]);

            LCD_SetCursor(1, 0);
            LCD_PrintString("Note:           ");
            LCD_SetCursor(1, 6);
            LCD_PrintString(piano_note_names[active]);
            LCD_PrintString(" ");
            LCD_PrintChar(key);
        } else {
            OLED_SetCursor(4, 14);
            OLED_PrintString("Press 1-8");
        }

        os_draw_piano_keyboard(active);

        OLED_Update();
        SysTick_DelayMs(30);
    }
}

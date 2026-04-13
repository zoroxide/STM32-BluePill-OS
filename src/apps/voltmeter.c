static void app_voltmeter(void) {
    ADC_Init(ADC_1);

    LCD_Clear();
    os_lcd_print_centered(0, "Voltmeter");

    uint16_t waveform[128];
    uint8_t wave_idx = 0;
    uint8_t wi;
    for (wi = 0; wi < 128; wi++) waveform[wi] = 0;

    while (1) {
        char key = Keypad_Scan();
        if (key == 'D') {
            while (Keypad_Scan() != KEYPAD_NO_KEY) {}
            SEG7_Clear();
            return;
        }

        uint32_t mv = ADC_ReadMillivolts(ADC_1, ADC_CH0);

        /* 7-segment shows voltage: e.g. 2.45V -> display "245" */
        SEG7_DisplayNumber((int16_t)(mv / 10));
        /* Show decimal point on digit 1 (tens position) */
        SEG7_DisplayDigit(2, (uint8_t)((mv / 1000) % 10), 1);
        uint16_t raw = ADC_Read(ADC_1, ADC_CH0);

        /* Store waveform sample */
        waveform[wave_idx] = (uint16_t)mv;
        wave_idx = (wave_idx + 1) & 0x7F; /* mod 128 */

        /* LCD line 2: voltage */
        LCD_SetCursor(1, 0);
        LCD_PrintString("                ");
        LCD_SetCursor(1, 0);
        LCD_PrintInt((int32_t)(mv / 1000));
        LCD_PrintChar('.');
        uint32_t frac = (mv % 1000) / 10;
        if (frac < 10) LCD_PrintChar('0');
        LCD_PrintInt((int32_t)frac);
        LCD_PrintString("V ADC:");
        LCD_PrintInt((int32_t)raw);

        /* Draw OLED */
        OLED_Clear();

        /* Title */
        OLED_SetCursor(4, 0);
        OLED_PrintString("Voltmeter");

        /* Voltage display */
        OLED_SetCursor(4, 10);
        OLED_PrintInt((int32_t)(mv / 1000));
        OLED_PrintChar('.');
        if (frac < 10) OLED_PrintChar('0');
        OLED_PrintInt((int32_t)frac);
        OLED_PrintString(" V");

        OLED_SetCursor(80, 10);
        OLED_PrintInt((int32_t)raw);

        /* Horizontal bar graph (0-3300 -> 0-120 px) */
        uint8_t bar_w = (uint8_t)((mv * 120) / 3300);
        OLED_DrawRect(4, 22, 120, 8, OLED_WHITE);
        if (bar_w > 0) {
            OLED_FillRect(4, 22, bar_w, 8, OLED_WHITE);
        }

        /* Waveform: bottom area y=34 to y=62 (28px height) */
        OLED_DrawLine(0, 34, 127, 34, OLED_WHITE);
        for (wi = 0; wi < 127; wi++) {
            uint8_t idx = (uint8_t)((wave_idx + wi) & 0x7F);
            uint8_t wy = (uint8_t)(62 - (waveform[idx] * 26) / 3300);
            if (wy < 35) wy = 35;
            if (wy > 62) wy = 62;
            OLED_DrawPixel(wi, wy, OLED_WHITE);
        }

        OLED_Update();
        SysTick_DelayMs(100);
    }
}

static const char * const piano_note_names[] = {
    "C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5"
};

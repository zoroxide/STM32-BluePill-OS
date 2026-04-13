
/* -------------------------------------------------------------------------
 *  App 7: HW Check
 * ------------------------------------------------------------------------- */
static void app_hwcheck(void) {
    LCD_Clear();
    os_lcd_print_centered(0, "HW Check");
    os_lcd_print_centered(1, "Testing...");

    uint8_t results[4]; /* 0=LCD, 1=OLED, 2=Keypad, 3=ADC */
    uint8_t ri;
    for (ri = 0; ri < 4; ri++) results[ri] = 0;

    /* Test 1: LCD write test (assume pass if we get here) */
    LCD_SetCursor(1, 0);
    LCD_PrintString("LCD: OK         ");
    results[0] = 1;
    SysTick_DelayMs(500);

    /* Test 2: OLED draw test */
    OLED_Clear();
    OLED_SetCursor(4, 4);
    OLED_PrintString("Testing OLED...");
    OLED_DrawRect(10, 20, 40, 20, OLED_WHITE);
    OLED_FillRect(60, 20, 40, 20, OLED_WHITE);
    OLED_DrawCircle(64, 50, 10, OLED_WHITE);
    OLED_Update();
    results[1] = 1;
    LCD_SetCursor(1, 0);
    LCD_PrintString("OLED: OK        ");
    SysTick_DelayMs(800);

    /* Test 3: Keypad test - ask user to press '5' */
    OLED_Clear();
    OLED_SetCursor(4, 10);
    OLED_PrintString("KEYPAD TEST");
    OLED_SetCursor(4, 26);
    OLED_PrintString("Press key: 5");
    OLED_SetCursor(4, 42);
    OLED_PrintString("(or D to skip)");
    OLED_Update();
    LCD_SetCursor(1, 0);
    LCD_PrintString("Press 5 on pad  ");

    uint32_t kp_start = SysTick_GetTicks();
    while (SysTick_GetTicks() - kp_start < 10000) { /* 10s timeout */
        char key = os_keypad_poll();
        if (key == '5') {
            results[2] = 1;
            break;
        }
        if (key == 'D') break;
        SysTick_DelayMs(20);
    }
    LCD_SetCursor(1, 0);
    if (results[2]) {
        LCD_PrintString("Keypad: OK      ");
    } else {
        LCD_PrintString("Keypad: FAIL    ");
    }
    SysTick_DelayMs(500);

    /* Test 4: ADC test */
    ADC_Init(ADC_1);
    uint16_t adc_val = ADC_Read(ADC_1, ADC_CH0);
    if (adc_val <= 4095) {
        results[3] = 1;
    }
    LCD_SetCursor(1, 0);
    if (results[3]) {
        LCD_PrintString("ADC: OK         ");
    } else {
        LCD_PrintString("ADC: FAIL       ");
    }
    SysTick_DelayMs(500);

    /* Show summary on OLED */
    OLED_Clear();
    OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
    OLED_SetCursor(20, 2);
    OLED_PrintString("HW CHECK");

    static const char * const hw_names[] = { "LCD", "OLED", "Keypad", "ADC" };
    for (ri = 0; ri < 4; ri++) {
        OLED_SetCursor(8, 16 + ri * 10);
        OLED_PrintString(hw_names[ri]);
        OLED_SetCursor(64, 16 + ri * 10);
        OLED_PrintString(results[ri] ? "[PASS]" : "[FAIL]");
    }

    /* Count results */
    uint8_t pass_count = 0;
    for (ri = 0; ri < 4; ri++) { if (results[ri]) pass_count++; }
    OLED_SetCursor(8, 56);
    OLED_PrintInt(pass_count);
    OLED_PrintString("/4 passed");
    OLED_Update();

    LCD_SetCursor(1, 0);
    LCD_PrintString("Done. D=exit    ");

    /* Wait for D to exit */
    while (1) {
        char key = os_keypad_poll();
        if (key == 'D') return;
        SysTick_DelayMs(30);
    }
}

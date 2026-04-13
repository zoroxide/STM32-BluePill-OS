
/* -------------------------------------------------------------------------
 *  App 4: Reaction Timer
 * ------------------------------------------------------------------------- */
static void app_reaction_timer(void) {
    rng_state = SysTick_GetTicks();

    LCD_Clear();
    os_lcd_print_centered(0, "Reaction Timer");

    while (1) {
        os_lcd_print_centered(1, "A:start D:exit");
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(8, 10);
        OLED_PrintString("REACTION TIMER");
        OLED_SetCursor(8, 30);
        OLED_PrintString("Press A to go!");
        OLED_Update();

        /* Wait for A or D */
        char key;
        while (1) {
            key = os_keypad_poll();
            if (key == 'A' || key == 'D') break;
            SysTick_DelayMs(20);
        }
        if (key == 'D') return;

        /* Show "Wait..." */
        OLED_Clear();
        OLED_SetCursor(30, 24);
        OLED_PrintString("Wait...");
        OLED_Update();
        os_lcd_print_centered(1, "Wait...");

        /* Random delay 1-4 seconds */
        uint32_t wait_time = 1000 + (os_rand() % 3000);
        uint32_t wait_start = SysTick_GetTicks();
        uint8_t  early = 0;

        while (SysTick_GetTicks() - wait_start < wait_time) {
            if (Keypad_Scan() != KEYPAD_NO_KEY) {
                early = 1;
                while (Keypad_Scan() != KEYPAD_NO_KEY) {}
                break;
            }
            SysTick_DelayMs(5);
        }

        if (early) {
            OLED_Clear();
            OLED_SetCursor(16, 24);
            OLED_PrintString("TOO EARLY!");
            OLED_SetCursor(8, 40);
            OLED_PrintString("Press A to retry");
            OLED_Update();
            os_lcd_print_centered(1, "Too early!");
            /* Wait for key */
            while (1) {
                key = os_keypad_poll();
                if (key == 'A' || key == 'D') break;
                SysTick_DelayMs(20);
            }
            if (key == 'D') return;
            continue;
        }

        /* Show GO! with buzzer + green LED */
        GPIO_SetPin(GRN_PORT, GRN_PIN);
        os_beep(1000, 50);
        OLED_Clear();
        OLED_FillRect(20, 10, 88, 44, OLED_WHITE);
        OLED_SetCursor(32, 26);
        OLED_PrintString("GO!");
        OLED_Update();
        os_lcd_print_centered(1, "GO! PRESS A!");

        uint32_t go_time = SysTick_GetTicks();

        /* Wait for any keypress */
        while (Keypad_Scan() == KEYPAD_NO_KEY) {}
        uint32_t react_time = SysTick_GetTicks() - go_time;
        while (Keypad_Scan() != KEYPAD_NO_KEY) {}
        GPIO_ClrPin(GRN_PORT, GRN_PIN);
        os_beep(600, 30);
        /* Show on 7-segment */
        SEG7_DisplayNumber((int16_t)(react_time > 999 ? 999 : react_time));

        /* Show result */
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(20, 8);
        OLED_PrintString("Your time:");
        OLED_SetCursor(20, 24);
        OLED_PrintInt((int32_t)react_time);
        OLED_PrintString(" ms");

        if (react_time < 200) {
            OLED_SetCursor(20, 40);
            OLED_PrintString("AMAZING!");
        } else if (react_time < 350) {
            OLED_SetCursor(20, 40);
            OLED_PrintString("Great!");
        } else {
            OLED_SetCursor(20, 40);
            OLED_PrintString("Keep trying!");
        }
        OLED_SetCursor(8, 54);
        OLED_PrintString("A:retry D:exit");
        OLED_Update();

        char rbuf[12];
        os_itoa((int32_t)react_time, rbuf, 12);
        LCD_SetCursor(1, 0);
        LCD_PrintString("                ");
        LCD_SetCursor(1, 0);
        LCD_PrintString(rbuf);
        LCD_PrintString(" ms");

        while (1) {
            key = os_keypad_poll();
            if (key == 'A' || key == 'D') break;
            SysTick_DelayMs(20);
        }
        if (key == 'D') return;
    }
}

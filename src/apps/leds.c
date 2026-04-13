} LED_Entry_t;

static LED_Entry_t os_leds[5];

static void os_leds_init_table(void) {
    os_leds[0] = (LED_Entry_t){ RED1_PORT, RED1_PIN, "RED1", 0 };
    os_leds[1] = (LED_Entry_t){ RED2_PORT, RED2_PIN, "RED2", 0 };
    os_leds[2] = (LED_Entry_t){ GRN_PORT,  GRN_PIN,  "GRN",  0 };
    os_leds[3] = (LED_Entry_t){ BLU_PORT,  BLU_PIN,  "BLU",  0 };
    os_leds[4] = (LED_Entry_t){ YLW_PORT,  YLW_PIN,  "YLW",  0 };
}

static void os_leds_apply(void) {
    uint8_t i;
    for (i = 0; i < 5; i++) {
        if (os_leds[i].state) GPIO_SetPin(os_leds[i].port, os_leds[i].pin);
        else                  GPIO_ClrPin(os_leds[i].port, os_leds[i].pin);
    }
}

static void app_leds(void) {
    uint8_t sel = 0;
    uint8_t mode = 0; /* 0=manual, 1=chase, 2=blink all */
    uint32_t last_tick = 0;
    uint8_t chase_idx = 0;
    uint8_t blink_state = 0;
    uint8_t i;

    os_leds_init_table();
    os_leds_apply();

    LCD_Clear();
    os_lcd_print_centered(0, "LED Control");

    while (1) {
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(20, 2);
        OLED_PrintString("LED CONTROL");
        OLED_DrawLine(0, 12, 127, 12, OLED_WHITE);

        /* Draw LED status */
        for (i = 0; i < 5; i++) {
            uint8_t y = 16 + i * 9;
            OLED_SetCursor(4, y);
            if (i == sel && mode == 0) OLED_PrintString("> ");
            else OLED_PrintString("  ");
            OLED_PrintString(os_leds[i].name);
            OLED_SetCursor(40, y);
            OLED_PrintString(os_leds[i].state ? "[ON ]" : "[OFF]");

            /* Visual LED circle */
            if (os_leds[i].state) {
                OLED_FillRect(80, y, 6, 6, OLED_WHITE);
            } else {
                OLED_DrawRect(80, y, 6, 6, OLED_WHITE);
            }
        }

        /* Mode indicator */
        OLED_SetCursor(4, 56);
        if (mode == 0) OLED_PrintString("Manual");
        else if (mode == 1) OLED_PrintString("Chase");
        else OLED_PrintString("Blink All");

        OLED_SetCursor(70, 56);
        OLED_PrintString("C:Mode");

        OLED_Update();

        /* Update LCD */
        LCD_SetCursor(1, 0);
        LCD_PrintString("                ");
        LCD_SetCursor(1, 0);
        if (mode == 0) {
            LCD_PrintString(os_leds[sel].name);
            LCD_PrintString(os_leds[sel].state ? " ON " : " OFF");
            LCD_SetCursor(1, 9);
            LCD_PrintString("A:Togl");
        } else if (mode == 1) {
            LCD_PrintString("Chase Pattern   ");
        } else {
            LCD_PrintString("Blink All       ");
        }

        /* Auto patterns */
        if (mode != 0) {
            uint32_t now = SysTick_GetTicks();
            if (now - last_tick > 200) {
                last_tick = now;
                if (mode == 1) {
                    /* Chase */
                    for (i = 0; i < 5; i++) os_leds[i].state = 0;
                    os_leds[chase_idx].state = 1;
                    chase_idx = (chase_idx + 1) % 5;
                } else {
                    /* Blink all */
                    blink_state = !blink_state;
                    for (i = 0; i < 5; i++) os_leds[i].state = blink_state;
                }
                os_leds_apply();
            }
        }

        /* Show current on 7-segment: LED count on */
        {
            uint8_t count = 0;
            for (i = 0; i < 5; i++) { if (os_leds[i].state) count++; }
            SEG7_DisplayNumber(count);
        }

        char key = os_keypad_poll();
        if (key == KEYPAD_NO_KEY) { SysTick_DelayMs(30); continue; }
        if (key == 'D') {
            /* Turn off all LEDs on exit */
            for (i = 0; i < 5; i++) os_leds[i].state = 0;
            os_leds_apply();
            SEG7_Clear();
            break;
        }
        if (key == 'C') {
            mode = (mode + 1) % 3;
            if (mode == 0) {
                /* Return to manual, turn all off */
                for (i = 0; i < 5; i++) os_leds[i].state = 0;
                os_leds_apply();
            }
        }
        if (mode == 0) {
            if (key == '*' || key == '2') { if (sel > 0) sel--; }
            if (key == '#' || key == '8') { if (sel < 4) sel++; }
            if (key == 'A') {
                os_leds[sel].state = !os_leds[sel].state;
                os_leds_apply();
                os_beep(os_leds[sel].state ? 600 : 300, 50);
            }
            /* Quick toggle by number */
            if (key >= '1' && key <= '5') {
                uint8_t idx = (uint8_t)(key - '1');
                os_leds[idx].state = !os_leds[idx].state;
                os_leds_apply();
                os_beep(os_leds[idx].state ? 600 : 300, 50);
            }
        }
    }
}

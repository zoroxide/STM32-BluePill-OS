
/* -------------------------------------------------------------------------
 *  App 8: Help
 * ------------------------------------------------------------------------- */

#define HELP_NUM_PAGES 4

static void app_help(void) {
    uint8_t page = 0;

    LCD_Clear();
    os_lcd_print_centered(0, "Help");

    while (1) {
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);

        switch (page) {
        case 0:
            OLED_SetCursor(20, 2);
            OLED_PrintString("CONTROLS");
            OLED_SetCursor(4, 14);
            OLED_PrintString("* = Scroll Up");
            OLED_SetCursor(4, 24);
            OLED_PrintString("# = Scroll Down");
            OLED_SetCursor(4, 34);
            OLED_PrintString("A = Select");
            OLED_SetCursor(4, 44);
            OLED_PrintString("D = Back/Exit");
            break;
        case 1:
            OLED_SetCursor(16, 2);
            OLED_PrintString("MENU NAV");
            OLED_SetCursor(4, 14);
            OLED_PrintString("1-8 = Quick sel");
            OLED_SetCursor(4, 24);
            OLED_PrintString("Pot = Scroll");
            OLED_SetCursor(4, 34);
            OLED_PrintString("* # = Up/Down");
            OLED_SetCursor(4, 44);
            OLED_PrintString("A = Enter app");
            break;
        case 2:
            OLED_SetCursor(8, 2);
            OLED_PrintString("GAME CONTROLS");
            OLED_SetCursor(4, 14);
            OLED_PrintString("2 = Up");
            OLED_SetCursor(4, 24);
            OLED_PrintString("4 = Left");
            OLED_SetCursor(4, 34);
            OLED_PrintString("6 = Right");
            OLED_SetCursor(4, 44);
            OLED_PrintString("8 = Down");
            break;
        case 3:
            OLED_SetCursor(24, 2);
            OLED_PrintString("ABOUT");
            OLED_SetCursor(4, 16);
            OLED_PrintString("BluePill OS 1.0");
            OLED_SetCursor(4, 28);
            OLED_PrintString("STM32F103C8T6");
            OLED_SetCursor(4, 40);
            OLED_PrintString("64K Flash");
            OLED_SetCursor(4, 50);
            OLED_PrintString("20K SRAM");
            break;
        default:
            break;
        }

        /* Page indicator */
        OLED_SetCursor(48, 56);
        OLED_PrintInt(page + 1);
        OLED_PrintChar('/');
        OLED_PrintInt(HELP_NUM_PAGES);

        OLED_Update();

        /* LCD status */
        LCD_SetCursor(1, 0);
        LCD_PrintString("Page            ");
        LCD_SetCursor(1, 5);
        LCD_PrintInt(page + 1);
        LCD_PrintChar('/');
        LCD_PrintInt(HELP_NUM_PAGES);
        LCD_PrintString(" */#=nav");

        char key = KEYPAD_NO_KEY;
        while (key == KEYPAD_NO_KEY) {
            key = os_keypad_poll();
            SysTick_DelayMs(20);
        }

        if (key == 'D') return;
        if (key == '#' || key == '8') {
            if (page < HELP_NUM_PAGES - 1) page++;
        }
        if (key == '*' || key == '2') {
            if (page > 0) page--;
        }
    }
}

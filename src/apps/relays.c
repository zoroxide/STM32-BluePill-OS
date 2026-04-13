
/* -------------------------------------------------------------------------
 *  App 11: Relay Controller
 * ------------------------------------------------------------------------- */
static void app_relays(void) {
    uint8_t rel1 = 0, rel2 = 0;
    uint8_t sel = 0;

    LCD_Clear();
    os_lcd_print_centered(0, "Relay Control");

    while (1) {
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(16, 2);
        OLED_PrintString("RELAY CONTROL");
        OLED_DrawLine(0, 12, 127, 12, OLED_WHITE);

        /* Relay 1 */
        OLED_SetCursor(4, 20);
        if (sel == 0) OLED_PrintString("> ");
        else OLED_PrintString("  ");
        OLED_PrintString("Relay 1: ");
        OLED_PrintString(rel1 ? "ON " : "OFF");

        /* Visual relay box */
        if (rel1) OLED_FillRect(100, 18, 20, 10, OLED_WHITE);
        else      OLED_DrawRect(100, 18, 20, 10, OLED_WHITE);

        /* Relay 2 */
        OLED_SetCursor(4, 34);
        if (sel == 1) OLED_PrintString("> ");
        else OLED_PrintString("  ");
        OLED_PrintString("Relay 2: ");
        OLED_PrintString(rel2 ? "ON " : "OFF");

        if (rel2) OLED_FillRect(100, 32, 20, 10, OLED_WHITE);
        else      OLED_DrawRect(100, 32, 20, 10, OLED_WHITE);

        /* Controls */
        OLED_SetCursor(4, 50);
        OLED_PrintString("A:Toggle */# Sel");
        OLED_SetCursor(4, 58);
        OLED_PrintString("B:Both OFF D:Xit");

        OLED_Update();

        /* 7-segment shows relay states: rel2*10 + rel1 (e.g. "10" = R2 on, R1 off) */
        SEG7_DisplayNumber((int16_t)(rel2 * 10 + rel1));

        /* LCD */
        LCD_SetCursor(1, 0);
        LCD_PrintString("R1:");
        LCD_PrintString(rel1 ? "ON  " : "OFF ");
        LCD_PrintString("R2:");
        LCD_PrintString(rel2 ? "ON  " : "OFF ");

        char key = os_keypad_poll();
        if (key == KEYPAD_NO_KEY) { SysTick_DelayMs(30); continue; }
        if (key == 'D') {
            /* Safety: turn off relays on exit */
            GPIO_ClrPin(REL1_PORT, REL1_PIN);
            GPIO_ClrPin(REL2_PORT, REL2_PIN);
            SEG7_Clear();
            break;
        }
        if (key == '*' || key == '2') sel = 0;
        if (key == '#' || key == '8') sel = 1;
        if (key == '1') { sel = 0; }
        if (key == '2') { sel = 1; }
        if (key == 'A') {
            if (sel == 0) {
                rel1 = !rel1;
                if (rel1) GPIO_SetPin(REL1_PORT, REL1_PIN);
                else      GPIO_ClrPin(REL1_PORT, REL1_PIN);
            } else {
                rel2 = !rel2;
                if (rel2) GPIO_SetPin(REL2_PORT, REL2_PIN);
                else      GPIO_ClrPin(REL2_PORT, REL2_PIN);
            }
            os_beep(500, 50);
        }
        if (key == 'B') {
            rel1 = 0; rel2 = 0;
            GPIO_ClrPin(REL1_PORT, REL1_PIN);
            GPIO_ClrPin(REL2_PORT, REL2_PIN);
            os_beep(300, 100);
        }
    }
}

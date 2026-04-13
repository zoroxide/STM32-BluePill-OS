
/* -------------------------------------------------------------------------
 *  App 9: Buzzer Fun
 * ------------------------------------------------------------------------- */
static void app_buzzer(void) {
    static const char * const note_names[] = {
        "C4","D4","E4","F4","G4","A4","B4","C5"
    };
    /* Approximate frequencies for C4-C5 */
    static const uint16_t note_freqs[] = {
        262, 294, 330, 349, 392, 440, 494, 523
    };

    LCD_Clear();
    os_lcd_print_centered(0, "Buzzer Fun");
    os_lcd_print_centered(1, "1-8:Note A:Song");

    while (1) {
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(20, 2);
        OLED_PrintString("BUZZER FUN");
        OLED_DrawLine(0, 12, 127, 12, OLED_WHITE);

        OLED_SetCursor(4, 16);
        OLED_PrintString("1-8: Play note");
        OLED_SetCursor(4, 26);
        OLED_PrintString("A: Melody");
        OLED_SetCursor(4, 36);
        OLED_PrintString("B: Siren");
        OLED_SetCursor(4, 46);
        OLED_PrintString("C: Alarm");
        OLED_SetCursor(4, 56);
        OLED_PrintString("D: Exit");

        OLED_Update();

        char key = os_keypad_poll();
        if (key == KEYPAD_NO_KEY) { SysTick_DelayMs(30); continue; }
        if (key == 'D') break;

        if (key >= '1' && key <= '8') {
            uint8_t n = (uint8_t)(key - '1');
            LCD_SetCursor(1, 0);
            LCD_PrintString("Playing:        ");
            LCD_SetCursor(1, 9);
            LCD_PrintString(note_names[n]);
            os_beep(note_freqs[n], 300);
        } else if (key == 'A') {
            /* Simple melody: C D E F G A B C */
            uint8_t i;
            LCD_SetCursor(1, 0);
            LCD_PrintString("Melody...       ");
            for (i = 0; i < 8; i++) {
                os_beep(note_freqs[i], 200);
                SysTick_DelayMs(50);
            }
            for (i = 7; i > 0; i--) {
                os_beep(note_freqs[i], 200);
                SysTick_DelayMs(50);
            }
        } else if (key == 'B') {
            /* Siren: sweep up and down */
            uint16_t f;
            LCD_SetCursor(1, 0);
            LCD_PrintString("Siren...        ");
            for (f = 300; f < 1000; f += 20) {
                os_beep(f, 20);
            }
            for (f = 1000; f > 300; f -= 20) {
                os_beep(f, 20);
            }
        } else if (key == 'C') {
            /* Alarm beeps */
            uint8_t i;
            LCD_SetCursor(1, 0);
            LCD_PrintString("Alarm!          ");
            for (i = 0; i < 5; i++) {
                os_beep(800, 100);
                SysTick_DelayMs(100);
            }
        }
        LCD_SetCursor(1, 0);
        LCD_PrintString("1-8:Note A:Song ");
    }
}

/* -------------------------------------------------------------------------
 *  App 10: LED Controller
 * ------------------------------------------------------------------------- */

typedef struct {
    GPIO_Port_t port;
    GPIO_Pin_t  pin;
    const char *name;
    uint8_t     state;

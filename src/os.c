/* OS Utilities */
static uint32_t rng_state;

static uint32_t os_rand(void) {
    rng_state = rng_state * 1664525 + 1013904223;
    return rng_state;
}

static void os_lcd_clear_line(uint8_t row) {
    LCD_SetCursor(row, 0);
    LCD_PrintString("                ");
}

static void os_lcd_print_centered(uint8_t row, const char *str) {
    uint8_t len = 0;
    const char *p = str;
    while (*p++) { len++; }
    uint8_t col = 0;
    if (len < 16) { col = (16 - len) / 2; }
    os_lcd_clear_line(row);
    LCD_SetCursor(row, col);
    LCD_PrintString(str);
}

static char os_keypad_poll(void) {
    char key = Keypad_Scan();
    if (key != KEYPAD_NO_KEY) {
        /* wait for release */
        while (Keypad_Scan() != KEYPAD_NO_KEY) {}
        delay(10000);
    }
    return key;
}

static void os_itoa(int32_t val, char *buf, uint8_t buflen) {
    if (buflen == 0) return;
    char tmp[12];
    uint8_t i = 0;
    int32_t v = val;
    uint8_t neg = 0;
    if (v < 0) { neg = 1; v = -v; }
    if (v == 0) { tmp[i++] = '0'; }
    while (v > 0 && i < 11) {
        tmp[i++] = '0' + (char)(v % 10);
        v /= 10;
    }
    if (neg && i < 11) { tmp[i++] = '-'; }
    uint8_t j = 0;
    while (i > 0 && j < buflen - 1) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

static void os_beep(uint32_t freq_hz, uint32_t duration_ms) {
    uint32_t half_period = 500000UL / freq_hz;
    uint32_t cycles = (freq_hz * duration_ms) / 1000;
    uint32_t i;
    for (i = 0; i < cycles; i++) {
        GPIO_SetPin(BUZZ_PORT, BUZZ_PIN);
        delay(half_period);
        GPIO_ClrPin(BUZZ_PORT, BUZZ_PIN);
        delay(half_period);
    }
}

/* ---- App files ---- */
#include "apps/splash.c"
#include "apps/calculator.c"
#include "apps/snake.c"
#include "apps/reaction.c"
#include "apps/voltmeter.c"
#include "apps/piano.c"
#include "apps/buzzer.c"
#include "apps/leds.c"
#include "apps/relays.c"
#include "apps/hwcheck.c"
#include "apps/help.c"
#include "apps/settings.c"
#include "apps/graphics.c"
#include "apps/rfid.c"

/* ---- Menu ---- */
#define MENU_NUM_ITEMS 13

static const char * const menu_items[] = {
    "Calculator",
    "Snake",
    "React Timer",
    "Voltmeter",
    "Piano",
    "3D Graphics",
    "Buzzer Fun",
    "LED Control",
    "Relays",
    "RFID (RC522)",
    "HW Check",
    "Help",
    "Settings"
};

typedef void (*app_func_t)(void);

static const app_func_t menu_apps[] = {
    app_calculator,
    app_snake,
    app_reaction_timer,
    app_voltmeter,
    app_piano,
    app_fancy_graphics,
    app_buzzer,
    app_leds,
    app_relays,
    app_rfid,
    app_hwcheck,
    app_help,
    app_settings
};

static void os_main_menu(void) {
    uint8_t selection = 0;
    uint8_t scroll_top = 0; /* first visible item index */
    uint8_t visible = 6;    /* max items visible on OLED at once */
    uint8_t last_pot_sel = 0;

    ADC_Init(ADC_1);

    /* Read initial pot position so we can detect movement */
    {
        uint16_t pot = ADC_Read(ADC_1, ADC_CH0);
        last_pot_sel = (uint8_t)((uint32_t)pot * MENU_NUM_ITEMS / 4096);
        if (last_pot_sel >= MENU_NUM_ITEMS) last_pot_sel = MENU_NUM_ITEMS - 1;
    }

    while (1) {
        /* Read potentiometer -- only override selection if pot actually moved */
        uint16_t pot = ADC_Read(ADC_1, ADC_CH0);
        uint8_t pot_sel = (uint8_t)((uint32_t)pot * MENU_NUM_ITEMS / 4096);
        if (pot_sel >= MENU_NUM_ITEMS) pot_sel = MENU_NUM_ITEMS - 1;

        if (pot_sel != last_pot_sel) {
            selection = pot_sel;
            last_pot_sel = pot_sel;
        }

        /* Adjust scroll window */
        if (selection < scroll_top) scroll_top = selection;
        if (selection >= scroll_top + visible) scroll_top = selection - visible + 1;

        /* Draw OLED menu */
        OLED_Clear();
        OLED_SetCursor(20, 0);
        OLED_PrintString("MAIN MENU");
        OLED_DrawLine(0, 9, 127, 9, OLED_WHITE);

        uint8_t mi;
        for (mi = 0; mi < visible && (scroll_top + mi) < MENU_NUM_ITEMS; mi++) {
            uint8_t idx = scroll_top + mi;
            uint8_t y = 12 + mi * 9;

            if (idx == selection) {
                /* Highlight: inverted bar */
                OLED_FillRect(0, y, 128, 9, OLED_WHITE);
                /* Text will be black on white - just re-draw cursor */
                OLED_SetCursor(4, y + 1);
                /* Since we can't easily do inverted text, draw a marker */
            } else {
                OLED_SetCursor(4, y + 1);
            }
            OLED_SetCursor(4, y + 1);
            if (idx == selection) {
                OLED_PrintString("> ");
            } else {
                OLED_PrintString("  ");
            }
            OLED_PrintString(menu_items[idx]);
        }

        /* Scroll indicator */
        if (scroll_top > 0) {
            OLED_SetCursor(120, 12);
            OLED_PrintChar('^');
        }
        if (scroll_top + visible < MENU_NUM_ITEMS) {
            OLED_SetCursor(120, 56);
            OLED_PrintChar('v');
        }

        OLED_Update();

        /* LCD */
        LCD_SetCursor(0, 0);
        LCD_PrintString("== MAIN MENU == ");
        LCD_SetCursor(1, 0);
        LCD_PrintString("                ");
        LCD_SetCursor(1, 0);
        LCD_PrintString(menu_items[selection]);

        /* Input handling */
        char key = os_keypad_poll();
        if (key == KEYPAD_NO_KEY) {
            SysTick_DelayMs(50);
            continue;
        }

        if (key == '*') {
            /* Scroll up */
            if (selection > 0) selection--;
        } else if (key == '#') {
            /* Scroll down */
            if (selection < MENU_NUM_ITEMS - 1) selection++;
        } else if (key == 'A') {
            /* Select current item */
            menu_apps[selection]();
            /* Restore menu LCD on return */
            LCD_Clear();
            ADC_Init(ADC_1);
        } else if (key >= '1' && key <= '9') {
            /* Quick select 1-9 */
            uint8_t app_idx = (uint8_t)(key - '1');
            if (app_idx < MENU_NUM_ITEMS) {
                os_beep(500, 30);
                selection = app_idx;
                menu_apps[app_idx]();
                LCD_Clear();
                ADC_Init(ADC_1);
            }
        }
    }
}

/* -------------------------------------------------------------------------
 *  OS Entry Point
 * ------------------------------------------------------------------------- */
void os_run(void) {
    /* Boot beep */
    os_beep(600, 80);
    SysTick_DelayMs(50);
    os_beep(800, 80);
    SysTick_DelayMs(50);
    os_beep(1000, 120);

    /* Flash green LED to show boot */
    GPIO_SetPin(GRN_PORT, GRN_PIN);

    /* Show splash screen */
    app_splash();

    GPIO_ClrPin(GRN_PORT, GRN_PIN);

    /* Enter main menu (never returns) */
    os_main_menu();

    /* Should never reach here */
    while (1) {}
}

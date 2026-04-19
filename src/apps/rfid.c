
/* -------------------------------------------------------------------------
 *  App: RFID Scanner (MFRC522 / RC522)
 *  Sub-menu: Scan UID, Reader Info, Back
 * ------------------------------------------------------------------------- */

static const char rfid_hex_chars[] = "0123456789ABCDEF";

static uint8_t rfid_initialized = 0;

/* ---- Utility: print N bytes as colon-separated hex on OLED ---- */
static void rfid_oled_hex(const uint8_t *data, uint8_t len) {
    uint8_t i;
    for (i = 0; i < len; i++) {
        if (i > 0) OLED_PrintChar(':');
        OLED_PrintChar(rfid_hex_chars[(data[i] >> 4) & 0x0F]);
        OLED_PrintChar(rfid_hex_chars[data[i] & 0x0F]);
    }
}

/* ---- Sub-feature: Scan a card ---- */
static void rfid_scan_card(void) {
    uint8_t uid[RC522_UID_LEN];
    uint8_t anim_frame = 0;

    GPIO_SetPin(BLU_PORT, BLU_PIN);

    while (1) {
        /* Waiting screen */
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(16, 2);
        OLED_PrintString("= RFID SCAN =");
        OLED_SetCursor(4, 20);
        OLED_PrintString("Present card");

        OLED_SetCursor(8, 32);
        anim_frame = (anim_frame + 1) % 4;
        if (anim_frame >= 1) OLED_PrintChar('.');
        if (anim_frame >= 2) OLED_PrintChar('.');
        if (anim_frame >= 3) OLED_PrintChar('.');

        OLED_SetCursor(8, 52);
        OLED_PrintString("D: back");
        OLED_Update();

        /* Attempt a read */
        if (RC522_ReadUID(uid) == 0) {
            /* Card found - feedback */
            GPIO_ClrPin(BLU_PORT, BLU_PIN);
            GPIO_SetPin(GRN_PORT, GRN_PIN);
            os_beep(1200, 80);
            SysTick_DelayMs(50);
            os_beep(1500, 80);

            /* Put card to sleep so the same card doesn't retrigger
             * immediately; a new REQA is needed on the next scan. */
            RC522_Halt();

            OLED_Clear();
            OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
            OLED_SetCursor(16, 2);
            OLED_PrintString("CARD FOUND!");
            OLED_SetCursor(8, 18);
            OLED_PrintString("UID (");
            OLED_PrintInt(RC522_UID_LEN);
            OLED_PrintString(" bytes):");
            OLED_SetCursor(4, 30);
            rfid_oled_hex(uid, RC522_UID_LEN);

            /* Decimal representation (big-endian uint32) for quick keypad entry */
            OLED_SetCursor(4, 42);
            OLED_PrintString("Dec: ");
            {
                uint32_t v = ((uint32_t)uid[0] << 24) |
                             ((uint32_t)uid[1] << 16) |
                             ((uint32_t)uid[2] <<  8) |
                             ((uint32_t)uid[3]);
                OLED_PrintInt((int32_t)v);
            }

            OLED_SetCursor(8, 54);
            OLED_PrintString("Any key = back");
            OLED_Update();

            SysTick_DelayMs(300);
            GPIO_ClrPin(GRN_PORT, GRN_PIN);

            while (os_keypad_poll() == KEYPAD_NO_KEY) SysTick_DelayMs(30);
            return;
        }

        /* Check for exit */
        char key = os_keypad_poll();
        if (key == 'D') {
            GPIO_ClrPin(BLU_PORT, BLU_PIN);
            return;
        }
        SysTick_DelayMs(80);
    }
}

/* ---- Sub-feature: Reader Info ---- */
static void rfid_show_info(void) {
    uint8_t ver = RC522_GetVersion();

    const char *ver_name;
    if      (ver == RC522_VERSION_1)     ver_name = "v1.0 (0x91)";
    else if (ver == RC522_VERSION_2)     ver_name = "v2.0 (0x92)";
    else if (ver == RC522_VERSION_CLONE) ver_name = "clone (0xB2)";
    else                                  ver_name = "UNKNOWN!";

    OLED_Clear();
    OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
    OLED_SetCursor(20, 2);
    OLED_PrintString("MFRC522 INFO");

    OLED_SetCursor(4, 18);
    OLED_PrintString("VersionReg:");
    OLED_SetCursor(4, 28);
    OLED_PrintString("0x");
    OLED_PrintChar(rfid_hex_chars[(ver >> 4) & 0x0F]);
    OLED_PrintChar(rfid_hex_chars[ver & 0x0F]);
    OLED_PrintString("  ");
    OLED_PrintString(ver_name);

    OLED_SetCursor(4, 42);
    OLED_PrintString("Antenna: ON");

    OLED_SetCursor(4, 54);
    OLED_PrintString("Any key = back");
    OLED_Update();

    while (os_keypad_poll() == KEYPAD_NO_KEY) SysTick_DelayMs(30);
}

/* ---- Main RFID app entry point ---- */
static void app_rfid(void) {
    uint8_t menu_sel = 0;  /* 0=Scan, 1=Info, 2=Back */

    /* ---- One-time RC522 initialization ----
     *
     * NOTE: on entry SPI2 takes over PB13/14/15 which are shared with
     * the LCD's D6/D5/D4 lines.  LCD output will be garbled while this
     * app runs.  On exit we re-init the LCD so subsequent apps can use
     * it again. */
    if (!rfid_initialized) {
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(16, 24);
        OLED_PrintString("RC522 init...");
        OLED_Update();

        if (RC522_Init(&rc522_cfg) != 0) {
            OLED_Clear();
            OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
            OLED_SetCursor(8, 14);
            OLED_PrintString("RC522 INIT FAIL");
            OLED_SetCursor(8, 28);
            OLED_PrintString("VersionReg=0x");
            {
                uint8_t v = RC522_GetVersion();
                OLED_PrintChar(rfid_hex_chars[(v >> 4) & 0x0F]);
                OLED_PrintChar(rfid_hex_chars[v & 0x0F]);
            }
            OLED_SetCursor(8, 42);
            OLED_PrintString("Check wiring!");
            OLED_SetCursor(8, 54);
            OLED_PrintString("D: back");
            OLED_Update();

            os_beep(200, 500);
            GPIO_SetPin(RED1_PORT, RED1_PIN);

            while (1) {
                char key = os_keypad_poll();
                if (key == 'D') {
                    GPIO_ClrPin(RED1_PORT, RED1_PIN);
                    LCD_Init(&lcd_pins);  /* restore LCD pins */
                    return;
                }
                SysTick_DelayMs(30);
            }
        }

        /* Success */
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(16, 16);
        OLED_PrintString("RC522 Ready!");
        OLED_SetCursor(8, 32);
        OLED_PrintString("Ver=0x");
        {
            uint8_t v = RC522_GetVersion();
            OLED_PrintChar(rfid_hex_chars[(v >> 4) & 0x0F]);
            OLED_PrintChar(rfid_hex_chars[v & 0x0F]);
        }
        OLED_Update();
        os_beep(1000, 100);
        SysTick_DelayMs(600);

        rfid_initialized = 1;
    }

    /* ---- Sub-menu loop ---- */
    while (1) {
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(16, 2);
        OLED_PrintString("= RFID MENU =");

        OLED_SetCursor(8, 18);
        OLED_PrintString(menu_sel == 0 ? "> Scan Card" : "  Scan Card");
        OLED_SetCursor(8, 30);
        OLED_PrintString(menu_sel == 1 ? "> Reader Info" : "  Reader Info");
        OLED_SetCursor(8, 42);
        OLED_PrintString(menu_sel == 2 ? "> Back" : "  Back");

        OLED_SetCursor(4, 56);
        OLED_PrintString("*/# A:sel D:back");
        OLED_Update();

        char key = os_keypad_poll();
        if (key == KEYPAD_NO_KEY) { SysTick_DelayMs(30); continue; }

        if (key == '*') {
            menu_sel = (menu_sel == 0) ? 2 : (uint8_t)(menu_sel - 1);
            continue;
        }
        if (key == '#') {
            menu_sel = (uint8_t)((menu_sel + 1) % 3);
            continue;
        }
        if (key == 'D') {
            /* Restore LCD pins before leaving so main menu LCD works again. */
            LCD_Init(&lcd_pins);
            return;
        }
        if (key == 'A') {
            switch (menu_sel) {
                case 0: rfid_scan_card(); break;
                case 1: rfid_show_info(); break;
                case 2:
                    LCD_Init(&lcd_pins);
                    return;
            }
        }
    }
}


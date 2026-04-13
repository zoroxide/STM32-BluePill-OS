
/* -------------------------------------------------------------------------
 *  App: NFC Reader/Writer (PN532)
 *  Sub-menu with Scan, Receive (Read NDEF), Send URL, Back
 * ------------------------------------------------------------------------- */

static const char hex_chars[] = "0123456789ABCDEF";

static uint8_t nfc_initialized = 0;

/* NDEF data for "https://www.futureacademyegypt.com/" */
/* Block 4: TLV header + NDEF record start + URI prefix + "futureac" + "a" */
static const uint8_t ndef_block4[16] = {
    0x03, 0x1C,                         /* NDEF TLV, length=28 */
    0xD1, 0x01, 0x18, 0x55,             /* NDEF record: MB|ME|SR|TNF=1, type_len=1, payload=24, type='U' */
    0x02,                               /* URI code: https://www. */
    'f','u','t','u','r','e','a','c',    /* "futureac" */
    'a'                                 /* "a" (start of "academy") */
};
/* Block 5: rest of URL + terminator */
static const uint8_t ndef_block5[16] = {
    'd','e','m','y','e','g','y','p',    /* "demyegyp" */
    't','.','c','o','m','/',            /* "t.com/" */
    0xFE,                               /* NDEF terminator TLV */
    0x00                                /* padding */
};

/* URI prefix decode table */
static const char * const uri_prefixes[] = {
    "",              /* 0x00 */
    "http://www.",   /* 0x01 */
    "https://www.",  /* 0x02 */
    "http://",       /* 0x03 */
    "https://"       /* 0x04 */
};

/**
 * @brief  Print a byte array as colon-separated hex on the OLED
 *         at the current cursor position (e.g. "04:A3:2B").
 */
static void nfc_print_hex(const uint8_t *data, uint8_t len) {
    uint8_t i;
    for (i = 0; i < len; i++) {
        if (i > 0) OLED_PrintChar(':');
        OLED_PrintChar(hex_chars[(data[i] >> 4) & 0x0F]);
        OLED_PrintChar(hex_chars[data[i] & 0x0F]);
    }
}

/**
 * @brief  Print a byte array as colon-separated hex on the LCD
 *         at the current cursor position.  Max 5 bytes to fit 16 cols.
 */
static void nfc_lcd_hex(const uint8_t *data, uint8_t len) {
    uint8_t i;
    uint8_t show = (len > 5) ? 5 : len;
    for (i = 0; i < show; i++) {
        if (i > 0) LCD_PrintChar(':');
        LCD_PrintChar(hex_chars[(data[i] >> 4) & 0x0F]);
        LCD_PrintChar(hex_chars[data[i] & 0x0F]);
    }
    if (len > 5) LCD_PrintString("..");
}

/* ---- Sub-feature: Scan Card ---- */
static void nfc_scan_card(void) {
    uint8_t uid[PN532_MAX_UID_LEN];
    uint8_t uid_len = 0;
    uint8_t anim_frame = 0;

    GPIO_SetPin(BLU_PORT, BLU_PIN);

    while (1) {
        /* Draw scanning screen */
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(16, 2);
        OLED_PrintString("= NFC SCAN =");
        OLED_SetCursor(8, 20);
        OLED_PrintString("Waiting for card");

        /* Pulsing dots animation */
        OLED_SetCursor(8, 32);
        anim_frame = (anim_frame + 1) % 4;
        if (anim_frame >= 1) OLED_PrintChar('.');
        if (anim_frame >= 2) OLED_PrintChar('.');
        if (anim_frame >= 3) OLED_PrintChar('.');

        OLED_SetCursor(8, 52);
        OLED_PrintString("D: back");
        OLED_Update();

        LCD_SetCursor(0, 0);
        LCD_PrintString("NFC: Scanning   ");
        LCD_SetCursor(1, 0);
        LCD_PrintString("                ");

        /* Try to detect a card */
        if (PN532_ReadPassiveTarget(PN532_BAUD_106, uid, &uid_len) == 0) {
            /* Card found */
            GPIO_ClrPin(BLU_PORT, BLU_PIN);
            GPIO_SetPin(GRN_PORT, GRN_PIN);
            os_beep(1200, 80);
            SysTick_DelayMs(60);
            os_beep(1500, 80);
            SysTick_DelayMs(200);
            GPIO_ClrPin(GRN_PORT, GRN_PIN);

            /* Display UID on OLED */
            OLED_Clear();
            OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
            OLED_SetCursor(16, 2);
            OLED_PrintString("CARD FOUND!");
            OLED_SetCursor(8, 18);
            OLED_PrintString("UID (");
            OLED_PrintInt(uid_len);
            OLED_PrintString(" bytes):");
            OLED_SetCursor(4, 30);
            nfc_print_hex(uid, uid_len);
            OLED_SetCursor(8, 52);
            OLED_PrintString("Press any key");
            OLED_Update();

            /* Display UID on LCD */
            LCD_SetCursor(0, 0);
            LCD_PrintString("NFC: Card Found ");
            LCD_SetCursor(1, 0);
            LCD_PrintString("UID:");
            nfc_lcd_hex(uid, uid_len);
            LCD_PrintString("  ");

            /* Wait for any key to return */
            while (os_keypad_poll() == KEYPAD_NO_KEY) {
                SysTick_DelayMs(30);
            }
            return;
        }

        /* Check for exit */
        char key = os_keypad_poll();
        if (key == 'D') {
            GPIO_ClrPin(BLU_PORT, BLU_PIN);
            return;
        }

        SysTick_DelayMs(50);
    }
}

/* ---- Sub-feature: Receive (Read NDEF) ---- */
static void nfc_receive(void) {
    uint8_t uid[PN532_MAX_UID_LEN];
    uint8_t uid_len = 0;
    uint8_t block4[16], block5[16];
    uint8_t default_key[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    uint8_t anim_frame = 0;

    /* Prompt to place card */
    GPIO_SetPin(BLU_PORT, BLU_PIN);

    while (1) {
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(12, 2);
        OLED_PrintString("= NFC RECEIVE =");
        OLED_SetCursor(8, 20);
        OLED_PrintString("Place card");

        OLED_SetCursor(8, 32);
        anim_frame = (anim_frame + 1) % 4;
        if (anim_frame >= 1) OLED_PrintChar('.');
        if (anim_frame >= 2) OLED_PrintChar('.');
        if (anim_frame >= 3) OLED_PrintChar('.');

        OLED_SetCursor(8, 52);
        OLED_PrintString("D: back");
        OLED_Update();

        LCD_SetCursor(0, 0);
        LCD_PrintString("NFC: Place card ");
        LCD_SetCursor(1, 0);
        LCD_PrintString("                ");

        char key = os_keypad_poll();
        if (key == 'D') {
            GPIO_ClrPin(BLU_PORT, BLU_PIN);
            return;
        }

        if (PN532_ReadPassiveTarget(PN532_BAUD_106, uid, &uid_len) == 0) {
            break;
        }
        SysTick_DelayMs(50);
    }

    GPIO_ClrPin(BLU_PORT, BLU_PIN);

    /* Card detected, read blocks 4 and 5 */
    OLED_Clear();
    OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
    OLED_SetCursor(8, 8);
    OLED_PrintString("Reading...");
    OLED_Update();

    /* Authenticate and read block 4 */
    if (PN532_MifareAuthA(4, default_key, uid, uid_len) != 0 ||
        PN532_MifareRead(4, block4) != 0) {
        GPIO_SetPin(RED1_PORT, RED1_PIN);
        os_beep(250, 400);
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(8, 20);
        OLED_PrintString("READ FAILED");
        OLED_SetCursor(8, 32);
        OLED_PrintString("Block 4 error");
        OLED_SetCursor(8, 48);
        OLED_PrintString("Press any key");
        OLED_Update();
        LCD_SetCursor(0, 0);
        LCD_PrintString("NFC Read Failed ");
        LCD_SetCursor(1, 0);
        LCD_PrintString("Block 4 error   ");
        while (os_keypad_poll() == KEYPAD_NO_KEY) SysTick_DelayMs(30);
        GPIO_ClrPin(RED1_PORT, RED1_PIN);
        return;
    }

    /* Authenticate and read block 5 */
    if (PN532_MifareAuthA(5, default_key, uid, uid_len) != 0 ||
        PN532_MifareRead(5, block5) != 0) {
        GPIO_SetPin(RED1_PORT, RED1_PIN);
        os_beep(250, 400);
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(8, 20);
        OLED_PrintString("READ FAILED");
        OLED_SetCursor(8, 32);
        OLED_PrintString("Block 5 error");
        OLED_SetCursor(8, 48);
        OLED_PrintString("Press any key");
        OLED_Update();
        LCD_SetCursor(0, 0);
        LCD_PrintString("NFC Read Failed ");
        LCD_SetCursor(1, 0);
        LCD_PrintString("Block 5 error   ");
        while (os_keypad_poll() == KEYPAD_NO_KEY) SysTick_DelayMs(30);
        GPIO_ClrPin(RED1_PORT, RED1_PIN);
        return;
    }

    /* Success reading both blocks */
    GPIO_SetPin(GRN_PORT, GRN_PIN);
    os_beep(1000, 80);

    /* Try to parse NDEF */
    uint8_t is_ndef = 0;
    uint8_t uri_code = 0;
    uint8_t payload_len = 0;
    uint8_t uri_start = 0;     /* offset in block4 where URI string starts */

    /* Check for NDEF TLV: block4[0] == 0x03 */
    if (block4[0] == 0x03) {
        uint8_t tlv_len = block4[1];
        /* Check NDEF record header: type='U' (0x55) for URI */
        if (tlv_len >= 4 && block4[5] == 0x55) {
            payload_len = block4[4];
            uri_code = block4[6];
            uri_start = 7;  /* URI string starts at block4[7] */
            is_ndef = 1;
        }
    }

    OLED_Clear();
    OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);

    if (is_ndef && uri_code <= 4) {
        /* NDEF URL found - display it */
        OLED_SetCursor(8, 2);
        OLED_PrintString("NDEF URL Found!");

        OLED_SetCursor(4, 16);
        /* Print URI prefix */
        OLED_PrintString(uri_prefixes[uri_code]);

        /* Print URI string from block4 (starting at uri_start) */
        uint8_t uri_str_len = payload_len - 1;  /* subtract URI code byte */
        uint8_t printed = 0;
        uint8_t bi;
        for (bi = uri_start; bi < 16 && printed < uri_str_len; bi++, printed++) {
            OLED_PrintChar((char)block4[bi]);
        }
        /* Continue from block5 if needed */
        for (bi = 0; bi < 16 && printed < uri_str_len; bi++, printed++) {
            if (block5[bi] == 0xFE) break;  /* NDEF terminator */
            OLED_PrintChar((char)block5[bi]);
        }

        /* Also show on LCD */
        LCD_SetCursor(0, 0);
        LCD_PrintString("NDEF URL:       ");
        LCD_SetCursor(1, 0);
        /* Print abbreviated URL on LCD */
        const char *pfx = uri_prefixes[uri_code];
        uint8_t pi = 0;
        uint8_t lcd_col = 0;
        while (pfx[pi] && lcd_col < 16) {
            LCD_PrintChar(pfx[pi]);
            pi++;
            lcd_col++;
        }
        printed = 0;
        for (bi = uri_start; bi < 16 && printed < uri_str_len && lcd_col < 16; bi++, printed++, lcd_col++) {
            LCD_PrintChar((char)block4[bi]);
        }
        for (bi = 0; bi < 16 && printed < uri_str_len && lcd_col < 16; bi++, printed++, lcd_col++) {
            if (block5[bi] == 0xFE) break;
            LCD_PrintChar((char)block5[bi]);
        }
    } else {
        /* Not NDEF - show raw hex data */
        OLED_SetCursor(8, 2);
        OLED_PrintString("Raw Block Data");

        /* Block 4 hex: two rows */
        OLED_SetCursor(4, 14);
        nfc_print_hex(block4, 8);
        OLED_SetCursor(4, 24);
        nfc_print_hex(block4 + 8, 8);

        /* Block 5 hex: two rows */
        OLED_SetCursor(4, 34);
        nfc_print_hex(block5, 8);
        OLED_SetCursor(4, 44);
        nfc_print_hex(block5 + 8, 8);

        /* ASCII representation on LCD */
        LCD_SetCursor(0, 0);
        LCD_PrintString("B4:");
        {
            uint8_t di;
            for (di = 0; di < 13 && di < 16; di++) {
                char ch = (char)block4[di];
                if (ch >= 0x20 && ch <= 0x7E)
                    LCD_PrintChar(ch);
                else
                    LCD_PrintChar('.');
            }
        }
        LCD_SetCursor(1, 0);
        LCD_PrintString("B5:");
        {
            uint8_t di;
            for (di = 0; di < 13 && di < 16; di++) {
                char ch = (char)block5[di];
                if (ch >= 0x20 && ch <= 0x7E)
                    LCD_PrintChar(ch);
                else
                    LCD_PrintChar('.');
            }
        }
    }

    OLED_SetCursor(8, 54);
    OLED_PrintString("Press any key");
    OLED_Update();

    GPIO_ClrPin(GRN_PORT, GRN_PIN);

    while (os_keypad_poll() == KEYPAD_NO_KEY) {
        SysTick_DelayMs(30);
    }
}

/* ---- Sub-feature: Send URL ---- */
static void nfc_send_url(void) {
    uint8_t uid[PN532_MAX_UID_LEN];
    uint8_t uid_len = 0;
    uint8_t default_key[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    uint8_t write_buf[16];
    uint8_t anim_frame = 0;

    /* Prompt to place card */
    GPIO_SetPin(BLU_PORT, BLU_PIN);

    while (1) {
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(12, 2);
        OLED_PrintString("= SEND URL =");
        OLED_SetCursor(4, 16);
        OLED_PrintString("futureacademy");
        OLED_SetCursor(4, 26);
        OLED_PrintString("egypt.com/");
        OLED_SetCursor(8, 40);
        OLED_PrintString("Place card");

        OLED_SetCursor(8, 50);
        anim_frame = (anim_frame + 1) % 4;
        if (anim_frame >= 1) OLED_PrintChar('.');
        if (anim_frame >= 2) OLED_PrintChar('.');
        if (anim_frame >= 3) OLED_PrintChar('.');

        OLED_Update();

        LCD_SetCursor(0, 0);
        LCD_PrintString("NFC: Write URL  ");
        LCD_SetCursor(1, 0);
        LCD_PrintString("Place card...   ");

        char key = os_keypad_poll();
        if (key == 'D') {
            GPIO_ClrPin(BLU_PORT, BLU_PIN);
            return;
        }

        if (PN532_ReadPassiveTarget(PN532_BAUD_106, uid, &uid_len) == 0) {
            break;
        }
        SysTick_DelayMs(50);
    }

    GPIO_ClrPin(BLU_PORT, BLU_PIN);

    /* Card detected - write NDEF data */
    OLED_Clear();
    OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
    OLED_SetCursor(8, 8);
    OLED_PrintString("Writing...");
    OLED_Update();

    /* Authenticate and write block 4 */
    {
        uint8_t i;
        for (i = 0; i < 16; i++) write_buf[i] = ndef_block4[i];
    }

    if (PN532_MifareAuthA(4, default_key, uid, uid_len) != 0) {
        goto write_fail;
    }
    if (PN532_MifareWrite(4, write_buf) != 0) {
        goto write_fail;
    }

    /* Authenticate and write block 5 */
    {
        uint8_t i;
        for (i = 0; i < 16; i++) write_buf[i] = ndef_block5[i];
    }

    if (PN532_MifareAuthA(5, default_key, uid, uid_len) != 0) {
        goto write_fail;
    }
    if (PN532_MifareWrite(5, write_buf) != 0) {
        goto write_fail;
    }

    /* Success */
    GPIO_SetPin(GRN_PORT, GRN_PIN);
    os_beep(1200, 80);
    SysTick_DelayMs(60);
    os_beep(1500, 80);

    OLED_Clear();
    OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
    OLED_SetCursor(16, 2);
    OLED_PrintString("URL Written!");
    OLED_SetCursor(4, 18);
    OLED_PrintString("https://www.");
    OLED_SetCursor(4, 28);
    OLED_PrintString("futureacademy");
    OLED_SetCursor(4, 38);
    OLED_PrintString("egypt.com/");
    OLED_SetCursor(8, 54);
    OLED_PrintString("Press any key");
    OLED_Update();

    LCD_SetCursor(0, 0);
    LCD_PrintString("URL Written OK! ");
    LCD_SetCursor(1, 0);
    LCD_PrintString("futureacademy...");

    GPIO_ClrPin(GRN_PORT, GRN_PIN);

    while (os_keypad_poll() == KEYPAD_NO_KEY) {
        SysTick_DelayMs(30);
    }
    return;

write_fail:
    GPIO_SetPin(RED1_PORT, RED1_PIN);
    os_beep(250, 400);

    OLED_Clear();
    OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
    OLED_SetCursor(8, 20);
    OLED_PrintString("WRITE FAILED!");
    OLED_SetCursor(8, 36);
    OLED_PrintString("Check card");
    OLED_SetCursor(8, 50);
    OLED_PrintString("Press any key");
    OLED_Update();

    LCD_SetCursor(0, 0);
    LCD_PrintString("NFC Write Fail! ");
    LCD_SetCursor(1, 0);
    LCD_PrintString("Check card      ");

    GPIO_ClrPin(RED1_PORT, RED1_PIN);

    while (os_keypad_poll() == KEYPAD_NO_KEY) {
        SysTick_DelayMs(30);
    }
}

/* ---- Main NFC app entry point ---- */
static void app_nfc(void) {
    uint8_t menu_sel = 0;  /* 0=Scan, 1=Receive, 2=Send URL, 3=Back */

    /* ---- One-time PN532 initialization ---- */
    if (!nfc_initialized) {
        LCD_Clear();
        os_lcd_print_centered(0, "NFC Reader");

        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(16, 24);
        OLED_PrintString("Initializing...");
        OLED_Update();

        if (PN532_Init(&nfc_cfg) != 0) {
            OLED_Clear();
            OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
            OLED_SetCursor(8, 20);
            OLED_PrintString("PN532 INIT FAIL");
            OLED_SetCursor(8, 36);
            OLED_PrintString("Check wiring!");
            OLED_Update();
            os_lcd_print_centered(1, "NFC Error!");
            os_beep(200, 500);
            GPIO_SetPin(RED1_PORT, RED1_PIN);

            while (1) {
                char key = os_keypad_poll();
                if (key == 'D') {
                    GPIO_ClrPin(RED1_PORT, RED1_PIN);
                    return;
                }
                SysTick_DelayMs(30);
            }
        }

        /* Read and display firmware version */
        uint8_t ic, ver, rev, support;
        if (PN532_GetFirmwareVersion(&ic, &ver, &rev, &support) == 0) {
            OLED_Clear();
            OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
            OLED_SetCursor(16, 8);
            OLED_PrintString("PN532 Ready!");
            OLED_SetCursor(8, 24);
            OLED_PrintString("IC: 0x");
            nfc_print_hex(&ic, 1);
            OLED_SetCursor(8, 34);
            OLED_PrintString("FW: ");
            OLED_PrintInt(ver);
            OLED_PrintChar('.');
            OLED_PrintInt(rev);
            OLED_SetCursor(8, 48);
            OLED_PrintString("Starting...");
            OLED_Update();

            os_lcd_print_centered(1, "PN532 OK");
            os_beep(1000, 100);
            SysTick_DelayMs(1500);
        }

        nfc_initialized = 1;
    }

    /* ---- Sub-menu loop ---- */
    while (1) {
        /* Draw sub-menu on OLED */
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(20, 2);
        OLED_PrintString("= NFC Menu =");

        OLED_SetCursor(8, 16);
        OLED_PrintString(menu_sel == 0 ? "> Scan Card" : "  Scan Card");

        OLED_SetCursor(8, 26);
        OLED_PrintString(menu_sel == 1 ? "> Receive" : "  Receive");

        OLED_SetCursor(8, 36);
        OLED_PrintString(menu_sel == 2 ? "> Send URL" : "  Send URL");

        OLED_SetCursor(8, 46);
        OLED_PrintString(menu_sel == 3 ? "> Back" : "  Back");

        OLED_SetCursor(4, 56);
        OLED_PrintString("*/# A:sel D:back");
        OLED_Update();

        LCD_SetCursor(0, 0);
        LCD_PrintString("NFC Menu        ");
        LCD_SetCursor(1, 0);
        switch (menu_sel) {
            case 0: LCD_PrintString(">Scan Card      "); break;
            case 1: LCD_PrintString(">Receive (Read) "); break;
            case 2: LCD_PrintString(">Send URL       "); break;
            case 3: LCD_PrintString(">Back           "); break;
        }

        char key = os_keypad_poll();
        if (key == KEYPAD_NO_KEY) {
            SysTick_DelayMs(30);
            continue;
        }

        if (key == '*') {
            if (menu_sel > 0) menu_sel--;
            else menu_sel = 3;
            continue;
        }
        if (key == '#') {
            if (menu_sel < 3) menu_sel++;
            else menu_sel = 0;
            continue;
        }
        if (key == 'D') {
            return;
        }
        if (key == 'A') {
            switch (menu_sel) {
                case 0: nfc_scan_card(); break;
                case 1: nfc_receive();   break;
                case 2: nfc_send_url();  break;
                case 3: return;
            }
        }
    }
}

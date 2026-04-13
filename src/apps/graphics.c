static void fancy_graphics_frame(void) {
    int16_t i;
    int16_t angle = frame * 3;

    OLED_Clear();

   /* --- Spinning 3D cube (wireframe) --- */
    /* 8 cube vertices in centered coords (-20..+20) */
    static const int8_t cube[8][3] = {
        {-20,-20,-20}, { 20,-20,-20}, { 20, 20,-20}, {-20, 20,-20},
        {-20,-20, 20}, { 20,-20, 20}, { 20, 20, 20}, {-20, 20, 20},
    };
    /* 12 edges as pairs of vertex indices */
    static const uint8_t edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},  /* back face  */
        {4,5},{5,6},{6,7},{7,4},  /* front face */
        {0,4},{1,5},{2,6},{3,7},  /* connecting */
    };

    /* Project and rotate all 8 vertices */
    int16_t px[8], py[8];
    int16_t sa = sin100(angle);
    int16_t ca = cos100(angle);
    int16_t sb = sin100(angle / 2);
    int16_t cb = cos100(angle / 2);

    for (i = 0; i < 8; i++) {
        /* Rotate around Y axis */
        int32_t x1 = ((int32_t)cube[i][0] * ca - (int32_t)cube[i][2] * sa) / 100;
        int32_t z1 = ((int32_t)cube[i][0] * sa + (int32_t)cube[i][2] * ca) / 100;
        int32_t y1 = cube[i][1];

        /* Rotate around X axis */
        int32_t y2 = (y1 * cb - z1 * sb) / 100;
        int32_t z2 = (y1 * sb + z1 * cb) / 100;

        /* Perspective projection (distance = 80) */
        int32_t depth = 80 + z2;
        if (depth < 20) { depth = 20; }

        px[i] = (int16_t)(32 + (x1 * 60) / depth);
        py[i] = (int16_t)(32 + (y2 * 60) / depth);
    }

    /* Draw 12 edges */
    for (i = 0; i < 12; i++) {
        uint8_t a = edges[i][0];
        uint8_t b = edges[i][1];
        OLED_DrawLine((uint8_t)px[a], (uint8_t)py[a],
                      (uint8_t)px[b], (uint8_t)py[b], OLED_WHITE);
    }

    /* --- Orbiting dots around the cube --- */
    for (i = 0; i < 3; i++) {
        int16_t dot_angle = angle + i * 120;
        int16_t dx = (int16_t)(32 + sin100(dot_angle) * 28 / 100);
        int16_t dy = (int16_t)(32 + cos100(dot_angle) * 28 / 100);
        OLED_FillRect((uint8_t)(dx - 1), (uint8_t)(dy - 1), 3, 3, OLED_WHITE);
    }

    /* --- Sine wave on the right side --- */
    for (i = 0; i < 50; i++) {
        int16_t wave_x = 75 + i;
        int16_t wave_y = 32 + (int16_t)(sin100((i * 12) + frame * 8) * 20 / 100);
        if (wave_y >= 0 && wave_y < 64) {
            OLED_DrawPixel((uint8_t)wave_x, (uint8_t)wave_y, OLED_WHITE);
        }
    }

    /* --- Bouncing text label --- */
    int16_t text_y = 0 + (sin100(frame * 5) * 4 / 100) + 4;
    OLED_SetCursor(75, (uint8_t)text_y);
    OLED_PrintString("Loay");
    OLED_SetCursor(81, (uint8_t)(text_y + 10));
    OLED_PrintString("Nour");

    /* --- Frame counter at bottom-right --- */
    OLED_SetCursor(100, 56);
    OLED_PrintInt(frame);

    OLED_Update();

    frame++;
    if (frame >= 120) { frame = 0; }
}

/* =========================================================================
 *  Raycasting FPS Engine (Wolfenstein-style)
 *  128x64 OLED, integer-only math using sin100/cos100
 *
 *  Controls: 2=forward, 8=backward, 4=turn left, 6=turn right, D=exit
 * ========================================================================= */

/* 8x8 map (1=wall, 0=empty) */
static const uint8_t rc_map[8][8] = {
    {1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,1},
    {1,0,1,0,1,0,0,1},
    {1,0,0,0,1,0,0,1},
    {1,0,1,0,0,0,1,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,1,0,1,0,1},
    {1,1,1,1,1,1,1,1},
};

static void app_raycaster(void) {
    /* Player position (fixed point: value * 100) */
    int32_t px = 150;  /* x=1.5 */
    int32_t py = 150;  /* y=1.5 */
    int16_t pa = 0;    /* angle in degrees */
    int16_t col;

    LCD_Clear();
    os_lcd_print_centered(0, "FPS Raycaster");

    while (1) {
        /* ---- Input ---- */
        char key = Keypad_Scan();
        if (key == 'D') {
            while (Keypad_Scan() != KEYPAD_NO_KEY) {}
            return;
        }
        if (key == '4') pa = (pa - 8 + 360) % 360;  /* Turn left  */
        if (key == '6') pa = (pa + 8) % 360;          /* Turn right */
        if (key == '2') {
            /* Move forward */
            int32_t nx = px + sin100(pa) * 6 / 100;
            int32_t ny = py - cos100(pa) * 6 / 100;
            if (rc_map[ny / 100][nx / 100] == 0) { px = nx; py = ny; }
        }
        if (key == '8') {
            /* Move backward */
            int32_t nx = px - sin100(pa) * 6 / 100;
            int32_t ny = py + cos100(pa) * 6 / 100;
            if (rc_map[ny / 100][nx / 100] == 0) { px = nx; py = ny; }
        }

        /* ---- Render ---- */
        OLED_Clear();

        /* FOV = 60 degrees, cast 128 rays (one per column) */
        int16_t start_angle = (pa - 30 + 360) % 360;

        for (col = 0; col < 128; col++) {
            int16_t ray_a = (start_angle + (col * 60) / 128) % 360;
            int32_t ray_sin = sin100(ray_a);
            int32_t ray_cos = cos100(ray_a);

            /* DDA raycasting with fixed-point steps */
            int32_t rx = px;
            int32_t ry = py;
            int16_t dist = 0;
            uint8_t hit = 0;
            int16_t step;

            for (step = 0; step < 200; step++) {
                rx += ray_sin * 2 / 100;
                ry -= ray_cos * 2 / 100;
                dist += 2;

                int32_t mx = rx / 100;
                int32_t my = ry / 100;

                if (mx < 0 || mx >= 8 || my < 0 || my >= 8) { hit = 1; break; }
                if (rc_map[my][mx] == 1) { hit = 1; break; }
            }

            if (hit) {
                /* Fix fisheye: multiply by cos of angle difference */
                int16_t diff = (ray_a - pa + 360) % 360;
                if (diff > 180) diff -= 360;
                int32_t cos_diff = cos100(diff);
                if (cos_diff < 10) cos_diff = 10;
                int32_t perp_dist = (int32_t)dist * cos_diff / 100;
                if (perp_dist < 5) perp_dist = 5;

                /* Wall height (taller when closer) */
                int16_t wall_h = (int16_t)(2000 / perp_dist);
                if (wall_h > 60) wall_h = 60;

                int16_t y_top = 32 - wall_h / 2;
                int16_t y_bot = 32 + wall_h / 2;
                if (y_top < 0) y_top = 0;
                if (y_bot > 63) y_bot = 63;

                /* Draw vertical wall strip */
                /* Dithered shading: skip pixels for distant walls */
                int16_t y;
                if (perp_dist < 80) {
                    /* Close: solid */
                    for (y = y_top; y <= y_bot; y++) {
                        OLED_DrawPixel((uint8_t)col, (uint8_t)y, OLED_WHITE);
                    }
                } else if (perp_dist < 160) {
                    /* Medium: every other pixel */
                    for (y = y_top; y <= y_bot; y++) {
                        if ((y + col) & 1)
                            OLED_DrawPixel((uint8_t)col, (uint8_t)y, OLED_WHITE);
                    }
                } else {
                    /* Far: sparse */
                    for (y = y_top; y <= y_bot; y++) {
                        if ((y & 3) == 0)
                            OLED_DrawPixel((uint8_t)col, (uint8_t)y, OLED_WHITE);
                    }
                }
            }
        }

        /* Floor dots for depth feel */
        for (col = 0; col < 128; col += 8) {
            OLED_DrawPixel((uint8_t)col, 60, OLED_WHITE);
        }

        /* Mini compass */
        OLED_SetCursor(0, 0);
        if (pa < 45 || pa >= 315)      OLED_PrintChar('N');
        else if (pa < 135)             OLED_PrintChar('E');
        else if (pa < 225)             OLED_PrintChar('S');
        else                           OLED_PrintChar('W');

        OLED_Update();

        LCD_SetCursor(1, 0);
        LCD_PrintString("Dir:");
        LCD_PrintInt(pa);
        LCD_PrintString("  ");
    }
}

/* =========================================================================
 *  3D Starfield - flying through stars
 * ========================================================================= */

static void app_starfield(void) {
    /* Star positions (x, y in -64..63 range, z = depth 1..100) */
    #define NUM_STARS 40
    int8_t  sx[NUM_STARS], sy[NUM_STARS];
    uint8_t sz[NUM_STARS];
    uint8_t i;

    /* Init random stars */
    rng_state = SysTick_GetTicks();
    for (i = 0; i < NUM_STARS; i++) {
        sx[i] = (int8_t)((os_rand() % 128) - 64);
        sy[i] = (int8_t)((os_rand() % 64) - 32);
        sz[i] = (uint8_t)(os_rand() % 100 + 1);
    }

    LCD_Clear();
    os_lcd_print_centered(0, "Starfield");
    os_lcd_print_centered(1, "D: Exit");

    while (1) {
        OLED_Clear();

        for (i = 0; i < NUM_STARS; i++) {
            /* Project star to screen */
            int16_t px = 64 + (int16_t)sx[i] * 64 / (int16_t)sz[i];
            int16_t py = 32 + (int16_t)sy[i] * 32 / (int16_t)sz[i];

            if (px >= 0 && px < 128 && py >= 0 && py < 64) {
                OLED_DrawPixel((uint8_t)px, (uint8_t)py, OLED_WHITE);
                /* Brighter (bigger) stars when close */
                if (sz[i] < 20) {
                    OLED_DrawPixel((uint8_t)(px + 1), (uint8_t)py, OLED_WHITE);
                    OLED_DrawPixel((uint8_t)px, (uint8_t)(py + 1), OLED_WHITE);
                }
            }

            /* Move star closer */
            sz[i] -= 2;
            if (sz[i] < 1 || sz[i] > 200) {
                /* Respawn far away */
                sx[i] = (int8_t)((os_rand() % 128) - 64);
                sy[i] = (int8_t)((os_rand() % 64) - 32);
                sz[i] = 100;
            }
        }

        OLED_Update();

        char key = Keypad_Scan();
        if (key == 'D') {
            while (Keypad_Scan() != KEYPAD_NO_KEY) {}
            return;
        }
        SysTick_DelayMs(30);
    }
}

/* =========================================================================
 *  3D Graphics Sub-Menu
 * ========================================================================= */
static void app_fancy_graphics(void) {
    uint8_t sel = 0;

    while (1) {
        OLED_Clear();
        OLED_SetCursor(16, 0);
        OLED_PrintString("3D GRAPHICS");
        OLED_DrawLine(0, 10, 127, 10, OLED_WHITE);

        OLED_SetCursor(4, 16);
        OLED_PrintString(sel == 0 ? "> " : "  ");
        OLED_PrintString("Spinning Cube");

        OLED_SetCursor(4, 26);
        OLED_PrintString(sel == 1 ? "> " : "  ");
        OLED_PrintString("FPS Raycaster");

        OLED_SetCursor(4, 36);
        OLED_PrintString(sel == 2 ? "> " : "  ");
        OLED_PrintString("Starfield");

        OLED_SetCursor(4, 52);
        OLED_PrintString("A:Select D:Back");
        OLED_Update();

        LCD_Clear();
        os_lcd_print_centered(0, "3D Graphics");
        static const char * const gfx_names[] = {
            "Spinning Cube", "FPS Raycaster", "Starfield"
        };
        os_lcd_print_centered(1, gfx_names[sel]);

        char key = os_keypad_poll();
        if (key == KEYPAD_NO_KEY) { SysTick_DelayMs(30); continue; }
        if (key == 'D') return;
        if (key == '*') { if (sel > 0) sel--; }
        if (key == '#') { if (sel < 2) sel++; }
        if (key == 'A') {
            if (sel == 0) {
                frame = 0;
                LCD_Clear();
                os_lcd_print_centered(0, gfx_names[0]);
                os_lcd_print_centered(1, "D: Exit");
                while (1) {
                    fancy_graphics_frame();
                    char k = Keypad_Scan();
                    if (k == 'D') { while (Keypad_Scan() != KEYPAD_NO_KEY) {} break; }
                }
            }
            if (sel == 1) { app_raycaster(); }
            if (sel == 2) { app_starfield(); }
        }
    }
}
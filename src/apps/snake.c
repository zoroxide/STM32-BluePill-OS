
/* -------------------------------------------------------------------------
 *  App 3: Snake Game
 * ------------------------------------------------------------------------- */

#define SNAKE_GRID_W    32
#define SNAKE_GRID_H    16
#define SNAKE_CELL_SIZE 4
#define SNAKE_MAX_LEN   128

static void app_snake(void) {
    rng_state = SysTick_GetTicks();

    uint8_t snake_x[SNAKE_MAX_LEN];
    uint8_t snake_y[SNAKE_MAX_LEN];
    uint8_t snake_len = 3;
    int8_t  dir_x = 1, dir_y = 0;
    uint8_t food_x, food_y;
    uint16_t score = 0;
    uint8_t  game_over = 0;
    uint16_t speed_ms = 200;
    uint8_t  si;

    /* Initialize snake in center */
    for (si = 0; si < 3; si++) {
        snake_x[si] = (uint8_t)(SNAKE_GRID_W / 2 - si);
        snake_y[si] = SNAKE_GRID_H / 2;
    }

    /* Place first food */
    food_x = (uint8_t)(os_rand() % SNAKE_GRID_W);
    food_y = (uint8_t)(os_rand() % SNAKE_GRID_H);

    LCD_Clear();
    os_lcd_print_centered(0, "Snake");
    LCD_SetCursor(1, 0);
    LCD_PrintString("Score: 0        ");

    while (!game_over) {
        uint32_t tick_start = SysTick_GetTicks();

        /* Poll input (non-blocking, allow multiple checks) */
        char key = Keypad_Scan();
        if (key != KEYPAD_NO_KEY) {
            if (key == '2' && dir_y != 1)  { dir_x = 0; dir_y = -1; }
            if (key == '8' && dir_y != -1) { dir_x = 0; dir_y = 1; }
            if (key == '4' && dir_x != 1)  { dir_x = -1; dir_y = 0; }
            if (key == '6' && dir_x != -1) { dir_x = 1; dir_y = 0; }
            if (key == 'D') return; /* Exit immediately */
            while (Keypad_Scan() != KEYPAD_NO_KEY) {}
        }

        /* Move snake: shift body */
        for (si = snake_len; si > 0; si--) {
            snake_x[si] = snake_x[si - 1];
            snake_y[si] = snake_y[si - 1];
        }
        /* New head position with wrapping */
        snake_x[0] = (uint8_t)((snake_x[0] + dir_x + SNAKE_GRID_W) % SNAKE_GRID_W);
        snake_y[0] = (uint8_t)((snake_y[0] + dir_y + SNAKE_GRID_H) % SNAKE_GRID_H);

        /* Check self-collision */
        for (si = 1; si < snake_len; si++) {
            if (snake_x[0] == snake_x[si] && snake_y[0] == snake_y[si]) {
                game_over = 1;
                break;
            }
        }

        /* Check food */
        if (snake_x[0] == food_x && snake_y[0] == food_y) {
            if (snake_len < SNAKE_MAX_LEN) {
                snake_len++;
            }
            score += 10;
            if (speed_ms > 80) speed_ms -= 5;
            /* New food */
            food_x = (uint8_t)(os_rand() % SNAKE_GRID_W);
            food_y = (uint8_t)(os_rand() % SNAKE_GRID_H);
            /* Update LCD score */
            LCD_SetCursor(1, 0);
            LCD_PrintString("Score:          ");
            LCD_SetCursor(1, 7);
            LCD_PrintInt(score);
            /* 7-segment shows score */
            SEG7_DisplayNumber((int16_t)score);
            /* Eat sound */
            os_beep(800, 30);
        }

        /* Draw */
        OLED_Clear();

        /* Draw food as a small filled block */
        OLED_FillRect(food_x * SNAKE_CELL_SIZE, food_y * SNAKE_CELL_SIZE,
                      SNAKE_CELL_SIZE, SNAKE_CELL_SIZE, OLED_WHITE);

        /* Draw snake */
        for (si = 0; si < snake_len; si++) {
            uint8_t px = snake_x[si] * SNAKE_CELL_SIZE;
            uint8_t py = snake_y[si] * SNAKE_CELL_SIZE;
            if (si == 0) {
                /* Head: filled */
                OLED_FillRect(px, py, SNAKE_CELL_SIZE, SNAKE_CELL_SIZE, OLED_WHITE);
            } else {
                /* Body: outline */
                OLED_DrawRect(px, py, SNAKE_CELL_SIZE, SNAKE_CELL_SIZE, OLED_WHITE);
            }
        }

        OLED_Update();

        /* Wait for remainder of tick */
        uint32_t elapsed = SysTick_GetTicks() - tick_start;
        if (elapsed < speed_ms) {
            SysTick_DelayMs(speed_ms - elapsed);
        }
    }

    /* Game over sound + red LED */
    GPIO_SetPin(RED1_PORT, RED1_PIN);
    os_beep(400, 100); os_beep(300, 100); os_beep(200, 200);
    GPIO_ClrPin(RED1_PORT, RED1_PIN);

    /* Game over screen */
    OLED_Clear();
    OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
    OLED_SetCursor(20, 16);
    OLED_PrintString("GAME OVER!");
    OLED_SetCursor(20, 32);
    OLED_PrintString("Score: ");
    OLED_PrintInt(score);
    OLED_SetCursor(10, 48);
    OLED_PrintString("Press any key");
    OLED_Update();

    LCD_SetCursor(1, 0);
    LCD_PrintString("GAME OVER!      ");

    Keypad_GetKey();
    SEG7_Clear();
}


static void app_calculator(void) {
    int32_t operand1 = 0;
    int32_t operand2 = 0;
    int32_t result = 0;
    uint8_t entering_op2 = 0;
    char    op = '+';
    uint8_t has_result = 0;
    char    num_buf[12];
    int32_t history[4];
    uint8_t hist_count = 0;
    uint8_t i;

    for (i = 0; i < 4; i++) history[i] = 0;

    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_PrintString("Calc: 0         ");
    LCD_SetCursor(1, 0);
    LCD_PrintString("Result: 0       ");

    while (1) {
        /* Draw OLED */
        OLED_Clear();
        OLED_DrawRect(0, 0, 128, 64, OLED_WHITE);
        OLED_SetCursor(4, 2);
        OLED_PrintString("= CALCULATOR =");

        /* Show current expression */
        OLED_SetCursor(4, 14);
        os_itoa(operand1, num_buf, 12);
        OLED_PrintString(num_buf);
        if (entering_op2) {
            OLED_PrintChar(' ');
            OLED_PrintChar(op);
            OLED_PrintChar(' ');
            os_itoa(operand2, num_buf, 12);
            OLED_PrintString(num_buf);
        }

        /* Show result */
        if (has_result) {
            OLED_SetCursor(4, 26);
            OLED_PrintString("= ");
            OLED_PrintInt(result);
        }

        /* History */
        OLED_SetCursor(4, 40);
        OLED_PrintString("History:");
        for (i = 0; i < hist_count && i < 4; i++) {
            OLED_SetCursor(8, 48 + i * 8);
            OLED_PrintInt(history[i]);
        }

        /* Key legend */
        OLED_SetCursor(80, 14);
        OLED_PrintString("C:op");
        OLED_SetCursor(80, 22);
        OLED_PrintString("A:= B:clr");

        OLED_Update();

        char key = os_keypad_poll();
        if (key == KEYPAD_NO_KEY) {
            SysTick_DelayMs(30);
            continue;
        }

        if (key == 'D') break; /* Exit */

        if (key >= '0' && key <= '9') {
            int32_t digit = key - '0';
            if (has_result) {
                /* Start fresh after result */
                operand1 = digit;
                operand2 = 0;
                entering_op2 = 0;
                has_result = 0;
            } else if (entering_op2) {
                if (operand2 >= 0 && operand2 < 100000) {
                    operand2 = operand2 * 10 + digit;
                }
            } else {
                if (operand1 >= 0 && operand1 < 100000) {
                    operand1 = operand1 * 10 + digit;
                }
            }
        } else if (key == 'C') {
            /* Cycle operator */
            if (!entering_op2) { entering_op2 = 1; }
            if (op == '+') op = '-';
            else if (op == '-') op = '*';
            else if (op == '*') op = '/';
            else op = '+';
        } else if (key == 'A') {
            /* Equals */
            if (op == '+') result = operand1 + operand2;
            else if (op == '-') result = operand1 - operand2;
            else if (op == '*') result = operand1 * operand2;
            else if (op == '/') {
                if (operand2 != 0) result = operand1 / operand2;
                else result = 0;
            }
            has_result = 1;
            /* Store in history */
            if (hist_count < 4) {
                history[hist_count++] = result;
            } else {
                for (i = 0; i < 3; i++) history[i] = history[i + 1];
                history[3] = result;
            }
            operand1 = result;
            operand2 = 0;
            entering_op2 = 0;
        } else if (key == 'B') {
            /* Clear */
            operand1 = 0;
            operand2 = 0;
            result = 0;
            entering_op2 = 0;
            has_result = 0;
            op = '+';
        }

        /* Update LCD */
        LCD_SetCursor(0, 0);
        LCD_PrintString("Calc:           ");
        LCD_SetCursor(0, 6);
        LCD_PrintInt(entering_op2 ? operand1 : operand1);
        if (entering_op2) {
            LCD_PrintChar(op);
            LCD_PrintInt(operand2);
        }
        LCD_SetCursor(1, 0);
        LCD_PrintString("Result:         ");
        LCD_SetCursor(1, 8);
        LCD_PrintInt(has_result ? result : 0);
    }
}

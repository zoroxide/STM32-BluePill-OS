/**
 * @file    OLED.c
 * @brief   SSD1306 I2C OLED Display Driver Implementation (128x64)
 *
 * Framebuffer-based driver for SSD1306 OLED displays over I2C.
 * All drawing operations modify an in-RAM framebuffer; call
 * OLED_Update() to flush the entire buffer to the display.
 *
 * @note    No stdlib dependencies beyond stdint.h.
 *          RAM usage: 1024 bytes for framebuffer + static state.
 * @version 1.0
 */

#include "OLED.h"

/* ───────────────────────── SSD1306 Command Definitions ───────────────────── */

#define SSD1306_CMD_DISPLAY_OFF         0xAE
#define SSD1306_CMD_DISPLAY_ON          0xAF
#define SSD1306_CMD_SET_CLOCK_DIV       0xD5
#define SSD1306_CMD_SET_MULTIPLEX       0xA8
#define SSD1306_CMD_SET_DISPLAY_OFFSET  0xD3
#define SSD1306_CMD_SET_START_LINE      0x40
#define SSD1306_CMD_CHARGE_PUMP         0x8D
#define SSD1306_CMD_SET_MEM_ADDR_MODE   0x20
#define SSD1306_CMD_SEG_REMAP           0xA1
#define SSD1306_CMD_COM_SCAN_DEC        0xC8
#define SSD1306_CMD_SET_COM_PINS        0xDA
#define SSD1306_CMD_SET_CONTRAST        0x81
#define SSD1306_CMD_SET_PRECHARGE       0xD9
#define SSD1306_CMD_SET_VCOMH           0xDB
#define SSD1306_CMD_DISPLAY_ALL_ON_RES  0xA4
#define SSD1306_CMD_NORMAL_DISPLAY      0xA6
#define SSD1306_CMD_SET_COL_ADDR        0x21
#define SSD1306_CMD_SET_PAGE_ADDR       0x22

#define SSD1306_CTRL_CMD                0x00
#define SSD1306_CTRL_DATA               0x40

/* ──────────────────────────── Static State ────────────────────────────────── */

/** @brief Framebuffer: 128 columns x 8 pages = 1024 bytes. */
static uint8_t oled_buffer[OLED_BUF_SIZE];

/** @brief I2C peripheral instance used for communication. */
static I2C_Peripheral_t oled_i2c;

/** @brief 7-bit I2C slave address of the OLED (typically 0x3C). */
static uint8_t oled_addr;

/** @brief Current text cursor X position (pixel column, 0-127). */
static uint8_t cursor_x;

/** @brief Current text cursor Y position (pixel row, 0-63). */
static uint8_t cursor_y;

/* ──────────────────────── 5x7 ASCII Font Table ───────────────────────────── */

/**
 * @brief Standard 5x7 ASCII font for characters 32 (' ') through 126 ('~').
 *
 * Each character is stored as 5 bytes (columns). Each byte represents a
 * 7-pixel-tall column with bit 0 at the top. HD44780-compatible layout.
 */
static const uint8_t OLED_FONT_5X7[][5] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, /* 32  ' ' */
    { 0x00, 0x00, 0x5F, 0x00, 0x00 }, /* 33  '!' */
    { 0x00, 0x07, 0x00, 0x07, 0x00 }, /* 34  '"' */
    { 0x14, 0x7F, 0x14, 0x7F, 0x14 }, /* 35  '#' */
    { 0x24, 0x2A, 0x7F, 0x2A, 0x12 }, /* 36  '$' */
    { 0x23, 0x13, 0x08, 0x64, 0x62 }, /* 37  '%' */
    { 0x36, 0x49, 0x55, 0x22, 0x50 }, /* 38  '&' */
    { 0x00, 0x05, 0x03, 0x00, 0x00 }, /* 39  ''' */
    { 0x00, 0x1C, 0x22, 0x41, 0x00 }, /* 40  '(' */
    { 0x00, 0x41, 0x22, 0x1C, 0x00 }, /* 41  ')' */
    { 0x14, 0x08, 0x3E, 0x08, 0x14 }, /* 42  '*' */
    { 0x08, 0x08, 0x3E, 0x08, 0x08 }, /* 43  '+' */
    { 0x00, 0x50, 0x30, 0x00, 0x00 }, /* 44  ',' */
    { 0x08, 0x08, 0x08, 0x08, 0x08 }, /* 45  '-' */
    { 0x00, 0x60, 0x60, 0x00, 0x00 }, /* 46  '.' */
    { 0x20, 0x10, 0x08, 0x04, 0x02 }, /* 47  '/' */
    { 0x3E, 0x51, 0x49, 0x45, 0x3E }, /* 48  '0' */
    { 0x00, 0x42, 0x7F, 0x40, 0x00 }, /* 49  '1' */
    { 0x42, 0x61, 0x51, 0x49, 0x46 }, /* 50  '2' */
    { 0x21, 0x41, 0x45, 0x4B, 0x31 }, /* 51  '3' */
    { 0x18, 0x14, 0x12, 0x7F, 0x10 }, /* 52  '4' */
    { 0x27, 0x45, 0x45, 0x45, 0x39 }, /* 53  '5' */
    { 0x3C, 0x4A, 0x49, 0x49, 0x30 }, /* 54  '6' */
    { 0x01, 0x71, 0x09, 0x05, 0x03 }, /* 55  '7' */
    { 0x36, 0x49, 0x49, 0x49, 0x36 }, /* 56  '8' */
    { 0x06, 0x49, 0x49, 0x29, 0x1E }, /* 57  '9' */
    { 0x00, 0x36, 0x36, 0x00, 0x00 }, /* 58  ':' */
    { 0x00, 0x56, 0x36, 0x00, 0x00 }, /* 59  ';' */
    { 0x08, 0x14, 0x22, 0x41, 0x00 }, /* 60  '<' */
    { 0x14, 0x14, 0x14, 0x14, 0x14 }, /* 61  '=' */
    { 0x00, 0x41, 0x22, 0x14, 0x08 }, /* 62  '>' */
    { 0x02, 0x01, 0x51, 0x09, 0x06 }, /* 63  '?' */
    { 0x32, 0x49, 0x79, 0x41, 0x3E }, /* 64  '@' */
    { 0x7E, 0x11, 0x11, 0x11, 0x7E }, /* 65  'A' */
    { 0x7F, 0x49, 0x49, 0x49, 0x36 }, /* 66  'B' */
    { 0x3E, 0x41, 0x41, 0x41, 0x22 }, /* 67  'C' */
    { 0x7F, 0x41, 0x41, 0x22, 0x1C }, /* 68  'D' */
    { 0x7F, 0x49, 0x49, 0x49, 0x41 }, /* 69  'E' */
    { 0x7F, 0x09, 0x09, 0x09, 0x01 }, /* 70  'F' */
    { 0x3E, 0x41, 0x49, 0x49, 0x7A }, /* 71  'G' */
    { 0x7F, 0x08, 0x08, 0x08, 0x7F }, /* 72  'H' */
    { 0x00, 0x41, 0x7F, 0x41, 0x00 }, /* 73  'I' */
    { 0x20, 0x40, 0x41, 0x3F, 0x01 }, /* 74  'J' */
    { 0x7F, 0x08, 0x14, 0x22, 0x41 }, /* 75  'K' */
    { 0x7F, 0x40, 0x40, 0x40, 0x40 }, /* 76  'L' */
    { 0x7F, 0x02, 0x0C, 0x02, 0x7F }, /* 77  'M' */
    { 0x7F, 0x04, 0x08, 0x10, 0x7F }, /* 78  'N' */
    { 0x3E, 0x41, 0x41, 0x41, 0x3E }, /* 79  'O' */
    { 0x7F, 0x09, 0x09, 0x09, 0x06 }, /* 80  'P' */
    { 0x3E, 0x41, 0x51, 0x21, 0x5E }, /* 81  'Q' */
    { 0x7F, 0x09, 0x19, 0x29, 0x46 }, /* 82  'R' */
    { 0x46, 0x49, 0x49, 0x49, 0x31 }, /* 83  'S' */
    { 0x01, 0x01, 0x7F, 0x01, 0x01 }, /* 84  'T' */
    { 0x3F, 0x40, 0x40, 0x40, 0x3F }, /* 85  'U' */
    { 0x1F, 0x20, 0x40, 0x20, 0x1F }, /* 86  'V' */
    { 0x3F, 0x40, 0x38, 0x40, 0x3F }, /* 87  'W' */
    { 0x63, 0x14, 0x08, 0x14, 0x63 }, /* 88  'X' */
    { 0x07, 0x08, 0x70, 0x08, 0x07 }, /* 89  'Y' */
    { 0x61, 0x51, 0x49, 0x45, 0x43 }, /* 90  'Z' */
    { 0x00, 0x7F, 0x41, 0x41, 0x00 }, /* 91  '[' */
    { 0x02, 0x04, 0x08, 0x10, 0x20 }, /* 92  '\' */
    { 0x00, 0x41, 0x41, 0x7F, 0x00 }, /* 93  ']' */
    { 0x04, 0x02, 0x01, 0x02, 0x04 }, /* 94  '^' */
    { 0x40, 0x40, 0x40, 0x40, 0x40 }, /* 95  '_' */
    { 0x00, 0x01, 0x02, 0x04, 0x00 }, /* 96  '`' */
    { 0x20, 0x54, 0x54, 0x54, 0x78 }, /* 97  'a' */
    { 0x7F, 0x48, 0x44, 0x44, 0x38 }, /* 98  'b' */
    { 0x38, 0x44, 0x44, 0x44, 0x20 }, /* 99  'c' */
    { 0x38, 0x44, 0x44, 0x48, 0x7F }, /* 100 'd' */
    { 0x38, 0x54, 0x54, 0x54, 0x18 }, /* 101 'e' */
    { 0x08, 0x7E, 0x09, 0x01, 0x02 }, /* 102 'f' */
    { 0x0C, 0x52, 0x52, 0x52, 0x3E }, /* 103 'g' */
    { 0x7F, 0x08, 0x04, 0x04, 0x78 }, /* 104 'h' */
    { 0x00, 0x44, 0x7D, 0x40, 0x00 }, /* 105 'i' */
    { 0x20, 0x40, 0x44, 0x3D, 0x00 }, /* 106 'j' */
    { 0x7F, 0x10, 0x28, 0x44, 0x00 }, /* 107 'k' */
    { 0x00, 0x41, 0x7F, 0x40, 0x00 }, /* 108 'l' */
    { 0x7C, 0x04, 0x18, 0x04, 0x78 }, /* 109 'm' */
    { 0x7C, 0x08, 0x04, 0x04, 0x78 }, /* 110 'n' */
    { 0x38, 0x44, 0x44, 0x44, 0x38 }, /* 111 'o' */
    { 0x7C, 0x14, 0x14, 0x14, 0x08 }, /* 112 'p' */
    { 0x08, 0x14, 0x14, 0x18, 0x7C }, /* 113 'q' */
    { 0x7C, 0x08, 0x04, 0x04, 0x08 }, /* 114 'r' */
    { 0x48, 0x54, 0x54, 0x54, 0x20 }, /* 115 's' */
    { 0x04, 0x3F, 0x44, 0x40, 0x20 }, /* 116 't' */
    { 0x3C, 0x40, 0x40, 0x20, 0x7C }, /* 117 'u' */
    { 0x1C, 0x20, 0x40, 0x20, 0x1C }, /* 118 'v' */
    { 0x3C, 0x40, 0x30, 0x40, 0x3C }, /* 119 'w' */
    { 0x44, 0x28, 0x10, 0x28, 0x44 }, /* 120 'x' */
    { 0x0C, 0x50, 0x50, 0x50, 0x3C }, /* 121 'y' */
    { 0x44, 0x64, 0x54, 0x4C, 0x44 }, /* 122 'z' */
    { 0x00, 0x08, 0x36, 0x41, 0x00 }, /* 123 '{' */
    { 0x00, 0x00, 0x7F, 0x00, 0x00 }, /* 124 '|' */
    { 0x00, 0x41, 0x36, 0x08, 0x00 }, /* 125 '}' */
    { 0x10, 0x08, 0x08, 0x10, 0x08 }, /* 126 '~' */
};

/* ──────────────────────── Private Helper Functions ────────────────────────── */

/**
 * @brief Send a single command byte to the SSD1306.
 * @param cmd  Command byte to send.
 */
static void OLED_Cmd(uint8_t cmd)
{
    uint8_t buf[2];
    buf[0] = SSD1306_CTRL_CMD;
    buf[1] = cmd;
    I2C_Write(oled_i2c, oled_addr, buf, 2);
}

/**
 * @brief Send a command followed by a parameter byte to the SSD1306.
 * @param cmd    Command byte.
 * @param param  Parameter byte.
 */
static void OLED_Cmd2(uint8_t cmd, uint8_t param)
{
    OLED_Cmd(cmd);
    OLED_Cmd(param);
}

/* ──────────────────────── Public API Implementation ──────────────────────── */

/**
 * @brief  Initialise the SSD1306 OLED display.
 *
 * Sends the full initialisation sequence for a 128x64 SSD1306 panel,
 * clears the framebuffer, and flushes it to the display.
 *
 * @param  i2c   I2C peripheral instance to use.
 * @param  addr  7-bit I2C slave address (typically 0x3C).
 */
void OLED_Init(I2C_Peripheral_t i2c, uint8_t addr)
{
    oled_i2c  = i2c;
    oled_addr = addr;
    cursor_x  = 0;
    cursor_y  = 0;

    /* Display off during configuration */
    OLED_Cmd(SSD1306_CMD_DISPLAY_OFF);

    /* Set clock divide ratio / oscillator frequency */
    OLED_Cmd2(SSD1306_CMD_SET_CLOCK_DIV, 0x80);

    /* Set multiplex ratio to 63 (64 lines) */
    OLED_Cmd2(SSD1306_CMD_SET_MULTIPLEX, 0x3F);

    /* Set display offset to 0 */
    OLED_Cmd2(SSD1306_CMD_SET_DISPLAY_OFFSET, 0x00);

    /* Set display start line to 0 */
    OLED_Cmd(SSD1306_CMD_SET_START_LINE);

    /* Enable charge pump */
    OLED_Cmd2(SSD1306_CMD_CHARGE_PUMP, 0x14);

    /* Set horizontal addressing mode */
    OLED_Cmd2(SSD1306_CMD_SET_MEM_ADDR_MODE, 0x00);

    /* Set segment re-map (column 127 mapped to SEG0) */
    OLED_Cmd(SSD1306_CMD_SEG_REMAP);

    /* Set COM output scan direction: remapped (bottom to top) */
    OLED_Cmd(SSD1306_CMD_COM_SCAN_DEC);

    /* Set COM pin hardware configuration for 128x64 */
    OLED_Cmd2(SSD1306_CMD_SET_COM_PINS, 0x12);

    /* Set contrast */
    OLED_Cmd2(SSD1306_CMD_SET_CONTRAST, 0xCF);

    /* Set pre-charge period */
    OLED_Cmd2(SSD1306_CMD_SET_PRECHARGE, 0xF1);

    /* Set VCOMH deselect level */
    OLED_Cmd2(SSD1306_CMD_SET_VCOMH, 0x40);

    /* Entire display ON, resume from GDDRAM content */
    OLED_Cmd(SSD1306_CMD_DISPLAY_ALL_ON_RES);

    /* Normal (non-inverted) display */
    OLED_Cmd(SSD1306_CMD_NORMAL_DISPLAY);

    /* Clear framebuffer and push to display */
    OLED_Clear();
    OLED_Update();

    /* Display on */
    OLED_Cmd(SSD1306_CMD_DISPLAY_ON);
}

/**
 * @brief  Clear the framebuffer (all pixels off) and reset the text cursor.
 */
void OLED_Clear(void)
{
    uint16_t i;
    for (i = 0; i < OLED_BUF_SIZE; i++) {
        oled_buffer[i] = 0x00;
    }
    cursor_x = 0;
    cursor_y = 0;
}

/**
 * @brief  Fill the framebuffer (all pixels on).
 */
void OLED_Fill(void)
{
    uint16_t i;
    for (i = 0; i < OLED_BUF_SIZE; i++) {
        oled_buffer[i] = 0xFF;
    }
}

/**
 * @brief  Flush the entire framebuffer to the SSD1306 via I2C.
 *
 * The framebuffer is transmitted in 16-byte chunks. Each chunk is
 * prefixed with a 0x40 data control byte as required by the SSD1306
 * I2C protocol.
 */
void OLED_Update(void)
{
    uint16_t i;
    uint8_t chunk[17]; /* 1 control byte + 16 data bytes */

    /* Set column address range: 0 to 127 */
    OLED_Cmd(SSD1306_CMD_SET_COL_ADDR);
    OLED_Cmd(0x00);
    OLED_Cmd(OLED_WIDTH - 1);

    /* Set page address range: 0 to 7 */
    OLED_Cmd(SSD1306_CMD_SET_PAGE_ADDR);
    OLED_Cmd(0x00);
    OLED_Cmd(OLED_PAGES - 1);

    /* Send framebuffer in 16-byte chunks */
    chunk[0] = SSD1306_CTRL_DATA;

    for (i = 0; i < OLED_BUF_SIZE; i += 16) {
        uint8_t j;
        for (j = 0; j < 16; j++) {
            chunk[j + 1] = oled_buffer[i + j];
        }
        I2C_Write(oled_i2c, oled_addr, chunk, 17);
    }
}

/**
 * @brief  Set or clear a single pixel in the framebuffer.
 *
 * @param  x      Pixel column (0 to OLED_WIDTH-1).
 * @param  y      Pixel row (0 to OLED_HEIGHT-1).
 * @param  color  OLED_WHITE (1) to set, OLED_BLACK (0) to clear.
 */
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) {
        return;
    }

    if (color) {
        oled_buffer[x + (y / 8) * OLED_WIDTH] |= (1 << (y & 7));
    } else {
        oled_buffer[x + (y / 8) * OLED_WIDTH] &= ~(1 << (y & 7));
    }
}

/**
 * @brief  Draw a straight line using Bresenham's algorithm.
 *
 * @param  x0     Start column.
 * @param  y0     Start row.
 * @param  x1     End column.
 * @param  y1     End row.
 * @param  color  OLED_WHITE or OLED_BLACK.
 */
void OLED_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color)
{
    int16_t dx  = (x1 > x0) ? (int16_t)(x1 - x0) : (int16_t)(x0 - x1);
    int16_t dy  = (y1 > y0) ? (int16_t)(y1 - y0) : (int16_t)(y0 - y1);
    int8_t  sx  = (x0 < x1) ? 1 : -1;
    int8_t  sy  = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    int16_t e2;

    for (;;) {
        OLED_DrawPixel(x0, y0, color);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        e2 = 2 * err;

        if (e2 > -dy) {
            err -= dy;
            x0  += sx;
        }

        if (e2 < dx) {
            err += dx;
            y0  += sy;
        }
    }
}

/**
 * @brief  Draw a rectangle outline in the framebuffer.
 *
 * @param  x      Top-left column.
 * @param  y      Top-left row.
 * @param  w      Width in pixels.
 * @param  h      Height in pixels.
 * @param  color  OLED_WHITE or OLED_BLACK.
 */
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    if (w == 0 || h == 0) {
        return;
    }

    /* Top and bottom edges */
    OLED_DrawLine(x, y, x + w - 1, y, color);
    OLED_DrawLine(x, y + h - 1, x + w - 1, y + h - 1, color);

    /* Left and right edges */
    OLED_DrawLine(x, y, x, y + h - 1, color);
    OLED_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

/**
 * @brief  Draw a filled rectangle in the framebuffer.
 *
 * @param  x      Top-left column.
 * @param  y      Top-left row.
 * @param  w      Width in pixels.
 * @param  h      Height in pixels.
 * @param  color  OLED_WHITE or OLED_BLACK.
 */
void OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    uint8_t i, j;

    for (i = 0; i < w; i++) {
        for (j = 0; j < h; j++) {
            OLED_DrawPixel(x + i, y + j, color);
        }
    }
}

/**
 * @brief  Set the text cursor position for subsequent print calls.
 *
 * @param  x  Column in pixels (0-127).
 * @param  y  Row in pixels (0-63).
 */
void OLED_SetCursor(uint8_t x, uint8_t y)
{
    cursor_x = x;
    cursor_y = y;
}

/**
 * @brief  Render a single ASCII character at the current cursor position.
 *
 * Uses the built-in 5x7 font. Each character occupies a 6-pixel-wide cell
 * (5 pixels for the glyph + 1 pixel spacing). The cursor automatically
 * advances. If the character would exceed the display width, the cursor
 * wraps to the next text row (8 pixels down).
 *
 * Handles newline ('\n') by moving the cursor to the start of the next
 * text row.
 *
 * @param  c  ASCII character to print (32-126).
 */
void OLED_PrintChar(char c)
{
    uint8_t i, j;
    uint8_t col_data;

    /* Handle newline */
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += 8;
        return;
    }

    /* Ignore non-printable characters */
    if (c < 32 || c > 126) {
        return;
    }

    /* Wrap to next line if character won't fit */
    if (cursor_x + 6 > OLED_WIDTH) {
        cursor_x = 0;
        cursor_y += 8;
    }

    /* Don't draw if off-screen vertically */
    if (cursor_y + 7 > OLED_HEIGHT) {
        return;
    }

    /* Render the 5 columns of the glyph */
    for (i = 0; i < 5; i++) {
        col_data = OLED_FONT_5X7[(uint8_t)c - 32][i];
        for (j = 0; j < 7; j++) {
            if (col_data & (1 << j)) {
                OLED_DrawPixel(cursor_x + i, cursor_y + j, OLED_WHITE);
            } else {
                OLED_DrawPixel(cursor_x + i, cursor_y + j, OLED_BLACK);
            }
        }
    }

    /* Clear the spacing column */
    for (j = 0; j < 7; j++) {
        OLED_DrawPixel(cursor_x + 5, cursor_y + j, OLED_BLACK);
    }

    /* Advance cursor by 6 pixels (5 glyph + 1 spacing) */
    cursor_x += 6;
}

/**
 * @brief  Print a null-terminated string at the current cursor position.
 *
 * @param  str  Pointer to the null-terminated string.
 */
void OLED_PrintString(const char *str)
{
    while (*str) {
        OLED_PrintChar(*str);
        str++;
    }
}

/**
 * @brief  Print a signed 32-bit integer as a decimal string.
 *
 * Handles negative numbers (prepends '-') and the special case of zero.
 * Does not use any standard library functions.
 *
 * @param  num  The integer value to print.
 */
void OLED_PrintInt(int32_t num)
{
    char buf[12]; /* -2147483648 is 11 chars + null */
    uint8_t i = 0;
    uint8_t j;
    uint32_t uval;
    char temp;

    if (num == 0) {
        OLED_PrintChar('0');
        return;
    }

    if (num < 0) {
        OLED_PrintChar('-');
        /* Handle INT32_MIN safely: cast to unsigned after negation trick */
        uval = (uint32_t)(-(num + 1)) + 1u;
    } else {
        uval = (uint32_t)num;
    }

    /* Build digits in reverse order */
    while (uval > 0) {
        buf[i++] = '0' + (char)(uval % 10);
        uval /= 10;
    }

    /* Reverse the digit string */
    for (j = 0; j < i / 2; j++) {
        temp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = temp;
    }

    /* Print each digit */
    for (j = 0; j < i; j++) {
        OLED_PrintChar(buf[j]);
    }
}

/**
 * @brief  Draw a circle outline using the midpoint circle algorithm.
 *
 * @param  cx     Centre column.
 * @param  cy     Centre row.
 * @param  r      Radius in pixels.
 * @param  color  OLED_WHITE or OLED_BLACK.
 */
void OLED_DrawCircle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t color)
{
    int16_t x = r;
    int16_t y = 0;
    int16_t d = 1 - (int16_t)r;

    if (r == 0) {
        OLED_DrawPixel(cx, cy, color);
        return;
    }

    while (x >= y) {
        OLED_DrawPixel(cx + x, cy + y, color);
        OLED_DrawPixel(cx - x, cy + y, color);
        OLED_DrawPixel(cx + x, cy - y, color);
        OLED_DrawPixel(cx - x, cy - y, color);
        OLED_DrawPixel(cx + y, cy + x, color);
        OLED_DrawPixel(cx - y, cy + x, color);
        OLED_DrawPixel(cx + y, cy - x, color);
        OLED_DrawPixel(cx - y, cy - x, color);

        y++;

        if (d <= 0) {
            d += 2 * y + 1;
        } else {
            x--;
            d += 2 * (y - x) + 1;
        }
    }
}

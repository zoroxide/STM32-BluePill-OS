# BluePill OS

A tiny menu-driven "OS" for the STM32F103C8T6 **Blue Pill** that bundles
thirteen mini-apps behind a keypad-navigable launcher on a 128×64 OLED.
Bare-metal C, no vendor HAL, no RTOS &mdash; just a hand-rolled register-level
driver layer on top of `arm-none-eabi-gcc`.

```
FLASH:  29556 B / 64 KB  (45 %)
RAM:     1532 B / 20 KB  ( 8 %)
```

---

## Hardware

| Peripheral            | Interface          | Pins                                       |
| --------------------- | ------------------ | ------------------------------------------ |
| 0.96" OLED (SSD1306)  | I²C1               | SCL=PB6, SDA=PB7                           |
| 16×2 character LCD    | GPIO (4-bit)       | RS=PA11, EN=PA8, D4-D7=PB15/14/13/12       |
| 4×4 matrix keypad     | GPIO               | rows=PA4-PA7, cols=PB11/PB10/PB1/PB0       |
| Buzzer                | GPIO               | PB4                                        |
| 2× relay              | GPIO               | REL1=PB5, REL2=PA3                         |
| 5× LED (R/R/G/B/Y)    | GPIO               | PA2/PA1/PC15/PC14/PC13                     |
| Potentiometer (ADC)   | ADC1 CH0           | PA0                                        |
| **RFID-RC522**        | SPI2               | SCK=PB13, MISO=PB14, MOSI=PB15, SS=PB9, RST=PB8 |

> **Pin conflict note:** The RC522 shares PB13/14/15 with LCD D6/D5/D4.
> Whenever the RFID app is active the 16×2 LCD displays garbage; on exit
> the app calls `LCD_Init()` to restore the GPIO mode.

Power: board is fed 5 V via USB or the 5 V pin; the RC522 module is
powered from the 3V3 regulator on the Blue Pill (do **not** tie its
VCC to 5 V).

---

## Apps

| #  | Name          | What it does                                          |
| -- | ------------- | ----------------------------------------------------- |
| 1  | Calculator    | 4-function calculator, keypad entry                   |
| 2  | Snake         | Classic snake on the OLED                             |
| 3  | React Timer   | Press-the-button reaction-time test                   |
| 4  | Voltmeter     | Displays ADC1-CH0 voltage (0&ndash;3.3 V)             |
| 5  | Piano         | Buzzer tone generator, keypad notes                   |
| 6  | 3D Graphics   | Rotating wireframe demo on the OLED                   |
| 7  | Buzzer Fun    | Preset melodies / sound effects                       |
| 8  | LED Control   | Manual toggle of all 5 LEDs                           |
| 9  | Relays        | Toggle REL1 / REL2                                    |
| 10 | RFID (RC522)  | Scan Mifare Classic cards, show UID (hex + decimal)   |
| 11 | HW Check      | Self-test of LEDs, buzzer, relays, ADC, keypad        |
| 12 | Help          | Keybind cheatsheet                                    |
| 13 | Settings      | Various tweaks (backlight, beep, etc.)                |

Menu keys: `*` / `#` scroll, `A` select, `D` back, digits `1`-`9`
quick-select.  A potentiometer on PA0 can also scrub the selection.

---

## Layout

```
├── HAL/                 Register-level STM32F103 HAL
│   ├── IO/              GPIO port/pin setup
│   ├── ISR/             SysTick + NVIC + delay helpers
│   ├── UART/            USART1..3, polling + ring-buffer RX
│   ├── SPI/             SPI1/SPI2 master
│   ├── I2C/             I2C1/I2C2 master
│   ├── ADC/             ADC1 single-shot
│   └── DAC/             DAC1 (unused - placeholder)
│
├── Drivers/             Off-chip peripheral drivers
│   ├── 16x2_LCD/        HD44780 4-bit GPIO driver
│   ├── 7_Segments/      Multiplexed 3-digit 7-seg driver
│   ├── I2C_OLED_Display/ SSD1306 128x64 driver with framebuffer
│   ├── Keypad/          4x4 matrix scanner
│   └── RC522_RFID/      MFRC522 ISO14443A (Mifare Classic) driver
│
├── src/
│   ├── main.c           Boot sequence + peripheral init
│   ├── startup.c        Reset_Handler, vector table, .bss/.data init
│   ├── os.c             Menu, app dispatch, #includes all apps
│   ├── config.h         Pin map & driver config structs
│   ├── utils/           delay loops, integer math helpers
│   └── apps/            Each app is a single .c file #included by os.c
│
├── Makefile             Unix / Git-Bash build
├── flash.bat            Windows build + OpenOCD flash script
└── stm32f103.ld         Linker script (64 KB Flash + 20 KB RAM)
```

The apps are `#include`d by `os.c` rather than compiled as separate
translation units — this lets every app see the same `static` helpers
(`os_keypad_poll`, `os_beep`, `os_itoa`, etc.) without any headers.

---

## Build

**Requirements**
- `arm-none-eabi-gcc` &ge; 13 (tested with Arm GNU Toolchain 15.2.Rel1)
- `openocd` for flashing
- An ST-Link V2 clone (3-wire SWD: SWDIO/SWCLK/GND)

**Unix / Git-Bash:**

```sh
make           # produces build/firmware.elf + build/firmware.bin
make flash     # runs openocd to flash via ST-Link
make clean     # wipe build/
```

**Windows (CMD/Powershell):**

```bat
.\flash.bat    :: compiles and flashes in one go
```

Both paths emit a "memory usage" summary on success.

---

## Driver Notes

### RC522 (Drivers/RC522_RFID/)
SPI mode 0, up to 10 MHz (driver runs at 2 MHz to stay comfortably
inside spec on an 8 MHz APB2).  The driver exposes:

```c
int8_t  RC522_Init(const RC522_Config_t *cfg);
uint8_t RC522_GetVersion(void);          // 0x91 / 0x92 / 0xB2
int8_t  RC522_IsCardPresent(void);       // REQA probe, non-destructive
int8_t  RC522_ReadUID(uint8_t uid[4]);   // REQA + anticoll + BCC check
void    RC522_Halt(void);                // put PICC to sleep
```

Initialisation configures the internal timer for a ~25 ms card-response
timeout, enables 100 % ASK modulation, installs the 0x6363 CRC preset,
and turns the antenna on.  `Init` returns -1 if `VersionReg` is not one
of the recognised values, which is the usual symptom of a wiring fault.

### HAL/UART ring buffer
`UART_EnableRxInterrupt` installs a 64-byte ring buffer filled from the
RXNE ISR.  `UART_RxAvailable` / `UART_RxRead` provide non-blocking
drain.  Polling-mode users of `UART_ReceiveByte` bypass it entirely.

### SysTick
`SysTick_Init(1000)` gives 1 kHz ticks; `SysTick_GetTicks()` /
`SysTick_DelayMs()` are the base for any ms-resolution timing.
`SysTick_SetCallback()` hooks a user callback into the ISR (kept short;
currently unused after the 7-seg multiplexer was disabled in `main.c`).

---

## Quick keybinding reference

| Context           | `*` / `#`      | `A`           | `B`  | `C`  | `D`     | 1-9            |
| ----------------- | -------------- | ------------- | ---- | ---- | ------- | -------------- |
| Main menu         | scroll         | open app      | &mdash; | &mdash; | &mdash; | jump to app N  |
| Any app sub-menu  | prev / next    | select        | &mdash; | &mdash; | back    | &mdash;        |
| Scan screens      | &mdash;        | &mdash;       | &mdash; | &mdash; | back    | &mdash;        |

---

## License

No license declared &mdash; personal / educational project.  Use at your
own risk; the Blue Pill clone PCBs and the RC522 modules are not always
wired correctly from the factory, so inspect before powering up.
